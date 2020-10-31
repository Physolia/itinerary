/*
    SPDX-FileCopyrightText: 2018 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

package org.kde.itinerary;

import org.qtproject.qt5.android.bindings.QtActivity;

import net.fortuna.ical4j.model.property.XProperty;

import androidx.core.content.FileProvider;

import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.provider.CalendarContract;
import android.os.Bundle;
import android.util.Log;

import java.io.*;
import java.util.*;


public class Activity extends QtActivity
{
    private static final String TAG = "org.kde.itinerary";

    public native void importReservation(String data);
    public native void importDavDroidJson(String data);
    public native void importFromIntent(Intent data);

    /** Check the calendar for with JSON-LD data.
     *  This assumes the custom property serialization format used by DavDroid.
     */
    public void checkCalendar()
    {
        Calendar startTime = Calendar.getInstance();
        startTime.add(Calendar.DAY_OF_YEAR, -5);
        Calendar endTime = Calendar.getInstance();
        endTime.add(Calendar.MONTH, 6);

        String[] eventColumns = new String[] { "uid2445", "title", "_id" };
        String[] propColumns = new String[] { "name", "value" };

        String eventSelection = "(( " + CalendarContract.Events.DTSTART + " >= " + startTime.getTimeInMillis()
            + " ) AND ( " + CalendarContract.Events.DTSTART + " <= " + endTime.getTimeInMillis() + " ))";

        try (Cursor cursor = getContentResolver().query(CalendarContract.Events.CONTENT_URI, eventColumns, eventSelection, null, null)) {
            while (cursor.moveToNext()) {
                if (cursor.getString(0) == null || !cursor.getString(0).startsWith("KIT-")) {
                    continue;
                }
                Log.i(TAG, cursor.getString(1));

                String propSelection = "(event_id == " + cursor.getInt(2) + ")";
                try (Cursor propCursor = getContentResolver().query(CalendarContract.ExtendedProperties.CONTENT_URI, propColumns, propSelection, null, null)) {
                    while (propCursor.moveToNext()) {
                        String propName = propCursor.getString(0);
                        String propValue = propCursor.getString(1);
                        if (propName == null || propValue == null) {
                            continue;
                        }

                        Log.i(TAG, propName);
                        if (propName.equals("unknown-property.v2") || propName.contains("vnd.ical4android.unknown-property")) {
                            importDavDroidJson(propValue);

                            // legacy, replaced by the above in Feb 2019, removing this eventually will allow us to remove the ical4j dependency
                        } else if (propName.equals("unknown-property")) {
                            ByteArrayInputStream bis = new ByteArrayInputStream(android.util.Base64.decode(propValue, android.util.Base64.NO_WRAP));
                            try {
                                ObjectInputStream ois = new ObjectInputStream(bis);
                                Object prop = ois.readObject();
                                if (prop instanceof XProperty) {
                                    importReservation(((XProperty) prop).getValue());
                                }
                            } catch (Exception e) {
                                Log.i(TAG, e.toString());
                            }
                        }
                    }
                }
            }
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        importFromIntent(intent);
    }

    public Uri openDocument(String filePath)
    {
        Log.i(TAG, filePath);
        File file = new File(filePath);
        return FileProvider.getUriForFile(this, "org.kde.itinerary.documentProvider", file);
    }

    /* Try to find attachment URLs from shared content from email applications.
     * - Fair Email adds content: URLs as EXTRA_STREAM, as a single Uri or an ArrayList<Uri>.
     */
    public String[] attachmentsForIntent(Intent intent)
    {
        try {
            Bundle extras = intent.getExtras();
            if (extras == null) {
                return null;
            }

            Object streamObj = extras.get(Intent.EXTRA_STREAM);
            if (streamObj == null) {
                return null;
            }

            Log.i(TAG, streamObj.getClass().toString());
            if (streamObj instanceof java.util.ArrayList) {
                ArrayList l = (ArrayList)streamObj;
                String[] result = new String[l.size()];
                for (int i = 0; i < l.size(); ++i) {
                    Object entry = l.get(i);
                    if (entry instanceof android.net.Uri) {
                        Uri uri = (Uri)entry;
                        result[i] = uri.toString();
                        continue;
                    }

                    Log.i(TAG, "unhandled array entry");
                    Log.i(TAG, entry.getClass().toString());
                    Log.i(TAG, entry.toString());
                }
                return result;
            }

            if (streamObj instanceof android.net.Uri) {
                Uri uri = (Uri)streamObj;
                String[] result = new String[1];
                result[0] = uri.toString();
                return result;
            }

            Log.i(TAG, "unhandled EXTRA_STREAM content type");
            Log.i(TAG, streamObj.getClass().toString());
            Log.i(TAG, streamObj.toString());
        } catch (java.lang.Exception e) {
            Log.i(TAG, e.toString());
        }
        return null;
    }
}
