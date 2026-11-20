# Avvio Windows

L'interfaccia usa Qt Quick/QML e `QGuiApplication`. Non è necessario collegare direttamente Qt Widgets. I vecchi sorgenti widget in Common non fanno parte dei target compilati da Morphora.

La compilazione non copia automaticamente le DLL Qt accanto all'eseguibile. Visual Studio può avviare l'app grazie al proprio ambiente; l'avvio da Esplora file richiede invece la distribuzione del runtime.

Da un ambiente di sviluppo Visual Studio x64:

```powershell
cmake --preset x64-debug
cmake --build --preset x64-debug-deploy
```

Il target `deploy-windows` compila Morphora e usa il `windeployqt` della stessa installazione Qt selezionata da CMake, includendo DLL, plugin e moduli QML necessari. Dopo questa operazione avviare `out/build/x64-debug/MorphMaster.exe`. Non spostare soltanto l'eseguibile: le dipendenze nella cartella fanno parte dell'applicazione.

Ripetere il deployment se cambiano versione Qt, dipendenze o import QML. Eventuali librerie richieste indirettamente da plugin Qt non implicano che l'interfaccia dell'app sia basata su QWidget.

Per Google Play usare screenshot rappresentativi della versione Android effettivamente distribuita. La build desktop è utile per preparare preset e configurazioni, ma non sostituisce il collaudo e le schermate Android.

## Screenshot per Google Play

### Anteprima del pulsante licenza

La build Windows permette di provare il pulsante fisso in fondo alla navigation rail:

| Scorciatoia | Anteprima |
| --- | --- |
| Ctrl+Alt+4 | Accesso completo: spunta verde |
| Ctrl+Alt+5 | Prova attiva: orologio giallo |
| Ctrl+Alt+6 | Accesso da verificare: punto esclamativo arancione |
| Ctrl+Alt+0 | Disattiva l'anteprima licenza |

Usare Ctrl+Alt+1 per il telefono e Ctrl+Alt+2 per il tablet, poi provare i tre stati in ciascun profilo. Lo stato scelto resta selezionato cambiando profilo o pagina. Fare clic sul simbolo per aprire il pannello licenza; Continue / Explore without MIDI torna all'app. Il testo del pannello segnala l'anteprima Windows e acquisto/ripristino sono disabilitati.

La simulazione non cambia licenze, acquisti o accesso MIDI e si azzera chiudendo l'app. Su Android il pulsante segue lo stato reale: anche Review access usa la spunta; pagamenti in sospeso e altri stati da verificare usano il punto esclamativo. Le scorciatoie di anteprima sono disponibili soltanto su Windows.

Nel profilo telefono 640×360, le otto posizioni di navigazione misurano circa 39,5 pixel in altezza e il pulsante licenza ne occupa 44. Verificare la comodità sul dispositivo prima di distribuire la modifica Android.

Nella build Windows:
- Ctrl+Alt+1: simulazione telefono orizzontale (640 x 360 logici).
- Ctrl+Alt+2: simulazione tablet orizzontale (1280 x 720 logici).
- F8: esporta l'area dell'app a 1920 x 1080, PNG RGB senza trasparenza, massimo 8 MB.

La cattura renderizza direttamente la scena Qt Quick alla risoluzione richiesta, mantenendo il layout mobile. Non include la cornice Windows. I file sono salvati nella cartella Immagini di Windows, sotto NaadaLab/Morphora/GooglePlay/phone oppure tablet, con nomi datati per non sovrascrivere le catture precedenti. Una finestra conferma il percorso dopo la cattura; chiuderla prima della successiva. Chiudere menu e attendere che spariscano eventuali suggerimenti prima di premere F8.

Usare almeno quattro schermate per ciascun layout. Non confondere questi screenshot con la feature graphic da 1024 x 500. Le catture vanno confrontate con l'app Android prima della pubblicazione, soprattutto per le schermate che contengono elementi specifici della piattaforma.
