# Publishing

## Release build of apk (QT Creator)

1. Create a keystore and add a signing key (In QT-creator: Project -> Build steps -> Build Android APK -> Sign package).
2. Sign the app using the key created in (1). It is necessary to have release version without debug server. 
3. Upload the APK to Google Play.
4. Download "Upload certificate" from Google Play Console -> App signing.
5. Add downloaded certificate to the keystore created in step (1) using command  `keytool -importcert -file <upload_cert.der> -keystore <keystorefile>`
It should prompt that "Certificate already exists in keystore under alias. Do you still want to add it? [no]:"
6. Type 'y' and press enter. You will receive a confirmation message.
7. For subsequent builds sign the app using the same process as in (2).

Note: Make sure you have updated `android:versionName="x.y.z"` and `android:versionCode="v"` in `AndroidManifest.xml` before any release update. 
Also check if you have target SDK set at least to version 26 (Android 8): `android:targetSdkVersion="26"`.

## Internal test release (Google Play Console )
To make an internal test release, open the Google Play Console -> Release management -> Internal test track byb clicking on "Manage" button.
1. Upload the apk you created in release mode with updated `android:versionCode="v"`.
2. Add release name and fill "What is new" section. 
3. Add testers email address to Manage testers panel with Feedback Channel.

The `Opt-in URL` address with published apk can be found on `Manage testers` panel or as a hyperlink on `Dashboard`. 
The publish process can take from several minutes to even sever hours (see publish status on overall Dashboard).

## Before the first release (Google Play Console )
* Fill all info and add all necessary files to `Store listing` in `Store presence` section. Do not forget about privacy policy.
* Fill form in `Pricing & distribution ` in `Store presence` section.


