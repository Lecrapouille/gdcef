Creer les scripts gdcef.gdns et gdcef.gdnlib peut importe les noms.
- gdcef.gdnlib refere la libgdcef.so comme biblo dynamique et libcef.so comme dependance
- gdcef.gdns refere gdcef.gdnlib
Si vous voyez "ERROR: does not have a library for the current platform." c'est que les chemins vers les lib ou vers gdcef.gdnlib ne sont pas bons.

godot-modules/demo/build est un pointeur vers godot-modules/build

Dans le scene graphe, creer un Node
puis lui selectionner le script GDNative gdcef.gdns

Bugs a la con:
```
LaunchProcess: failed to execvp:
/home/qq/chreage_workspace/godot-modules/demo/gdcef/godotcefsimple
GDCef::RenderHandler::GetViewRect
GDCef::RenderHandler::GetViewRect
GDCef::RenderHandler::GetViewRect
[GDCef] CreateBrowserSync has been called !
GDCef::GDCef() done
GDCef::DoMessageLoop()
[1216/041347.746815:ERROR:gpu_process_host.cc(973)] GPU process launch failed: error_code=1002
[1216/041347.746850:WARNING:gpu_process_host.cc(1292)] The GPU process has crashed 1 time(s)
[1216/041347.748935:ERROR:network_service_instance_impl.cc(638)] Network service crashed, restarting service.
```
=> Pensez a changer le path vers godotcefsimple


OS X: prerequis
Installer Xcode (12 Go). L'ouvrir au moins une fois pour finaliser l'installation.
Faire: sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
TODO: penser a linker avec cef_sandbox.a
FIXME: comment creer les 4 sous applications mac ?
