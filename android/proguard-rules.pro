# Qt calls Java through JNI and reflection. Preserve the framework's ABI,
# including our MidiAndroidBridge in the org.qtproject.qt.android package.
# Narrowing this rule requires an audit of the corresponding Qt native code.
-keep class org.qtproject.qt.** { *; }

# Entry points looked up by name in Common/src/licensing/LicenseAccess.cpp.
# Internal implementation and ReviewAccessToken remain eligible for R8.
-keep class com.naadalab.licensing.PlayLicenseBridge {
    public static void initialize(android.app.Activity);
    public static void resume();
    public static void activate();
    public static void restore();
    public static void buy();
    public static void review(java.lang.String);
    public static java.lang.String status();
}
