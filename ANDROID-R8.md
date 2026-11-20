# Android: R8 in Morphora e replica in Intona

## Configurazione

Morphora android/build.gradle abilita minifyEnabled per release con proguard-android-optimize.txt e android/proguard-rules.pro. Debug invariato. Resource shrinking disabilitato: Qt accede dinamicamente alle risorse. Non aggiungere dontwarn globale o dontobfuscate.

Regole: mantenere org.qtproject.qt.** (Qt e MidiAndroidBridge); mantenere soltanto gli entry point JNI pubblici di PlayLicenseBridge elencati in LicenseAccess.cpp. Il resto del codice licenze resta ottimizzabile. Verificare nuovamente le regole quando si aggiorna Qt o si aggiungono chiamate JNI/riflessione.

R8 riguarda Java/DEX, non offusca il codice C++ o QML e non sostituisce la verifica server degli acquisti.

## Verifiche build 4

Bundletool validate superato, versionCode 4, ARMv7 e ARM64 presenti. Mapping controllato: entry point licenze e classi Qt/MIDI conservati; le classi sintetiche del compilatore possono cambiare nome. Audit del pacchetto senza chiavi private o credenziale reviewer incorporata superato.

DEX non compresso: 7019684 -> 697224 byte (-90,07%). AAB: 64062493 -> 62312031 byte. Questi dati non sono il punteggio di ottimizzazione/offuscazione di Play Console, ne una misura di sicurezza dell'intera app.

Prove reali sulla build 4 ancora da eseguire: avvio, MIDI input/output, morphing, stato licenza conservato, restore online, riavvio offline; inoltre prova, acquisto test e review access su uno stato adatto.

## Replica in Intona

Dopo il collaudo di Morphora, integrare le stesse impostazioni nel build.gradle di Intona senza sovrascriverne le differenze. Copiare/adattare le regole dopo aver confrontato tutti i suoi entry point JNI e i package Java. Generare un nuovo AAB e archiviare il SUO mapping.txt; ripetere validate, audit e prove reali. Non riutilizzare il mapping di Morphora.

Documentazione: https://developer.android.com/topic/performance/app-optimization/enable-app-optimization
