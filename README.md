# KDE Itinerary

A digital travel assistant that supports you while traveling without compromising your privacy.

![KDE Itinerary timeline view](https://cdn.kde.org/screenshots/itinerary/kde-itinerary-timeline.png)

## Getting KDE Itinerary

### Flatpak

Stable flatpaks are available from [Flathub](https://flathub.org/apps/details/org.kde.itinerary).

Nightly flatpaks are available from KDE's Flatpak repository:

```
flatpak remote-add --if-not-exists kdeapps --from https://distribute.kde.org/kdeapps.flatpakrepo
flatpak install org.kde.itinerary
```

### Android

Release builds are available in KDE's [release F-Droid repository](https://cdn.kde.org/android/stable-releases/fdroid/repo/?fingerprint=13784BA6C80FF4E2181E55C56F961EED5844CEA16870D3B38D58780B85E1158F).

![Link to KDE's release F-Droid repository](doc/kde-fdroid-release-repository-link.png)

Nightly builds are available from KDE's [nightly F-Droid Repository](https://cdn.kde.org/android/fdroid/repo/?fingerprint=B3EBE10AFA6C5C400379B34473E843D686C61AE6AD33F423C98AF903F056523F).

![Link to KDE's nightly F-Droid repository](doc/kde-fdroid-release-repository-link.png)

Alternatively, you can download APKs directly from [Binary Factory](https://binary-factory.kde.org):
- Nightly builds for [arm64](https://binary-factory.kde.org/view/Android/job/Itinerary_Nightly_android-arm64/), [arm](https://binary-factory.kde.org/view/Android/job/Itinerary_Nightly_android-arm/), [x86_64](https://binary-factory.kde.org/view/Android/job/Itinerary_Nightly_android-x86_64/) and [x86](https://binary-factory.kde.org/view/Android/job/Itinerary_Nightly_android-x86/)
- Release builds for [arm64](https://binary-factory.kde.org/view/Android/job/Itinerary_Release_android-arm64/), [arm](https://binary-factory.kde.org/view/Android/job/Itinerary_Release_android-arm/), [x86_64](https://binary-factory.kde.org/view/Android/job/Itinerary_Release_android-x86_64/) and [x86](https://binary-factory.kde.org/view/Android/job/Itinerary_Release_android-x86/)

The version in the Play Store can currently not be updated due to Play Store policies.

## Contributing

General introduction: https://community.kde.org/Get_Involved

Building:
- Android: https://develop.kde.org/docs/packaging/android/building_applications/
- Othe platforms: https://community.kde.org/Get_Involved/development

Important external components:
- Travel document extractor engine: https://invent.kde.org/pim/kitinerary
- Public transport data access: https://invent.kde.org/libraries/kpublictransport
- Indoor map renderer: https://invent.kde.org/libraries/kosmindoormap

Matrix channel: [#itinerary:kde.org](https://matrix.to/#/#itinerary:kde.org)
