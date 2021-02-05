# NuGet package

Install from [NuGet gallery package]() (recommended):

```powershell
Install-Package ZeroTier.Sockets
```

Install from local package

```powershell
Install-Package ZeroTier.Sockets -Source C:\PathToThePackageDir\
```

Development notes

 - Microsoft's own documentation on multi-architecture nupkgs was outdated and Marco Siccardi seemed to have [the only correct instructions found anywhere on the internet](https://msicc.net/how-to-create-a-multi-architecture-nuget-package-from-a-uwp-class-library/) to accomplish this.