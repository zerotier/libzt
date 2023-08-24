# Nodejs libzt binding

## Build in development

```bash
./build.sh host
cd src/bindings/nodejs
npm run link
npm ci            (rerun after changes to native code)
npm run build     (rerun after changes to typescript)
npm test (TODO)
```

## Current features

- Typescript
- Subset of ZeroTierSockets.h directly accessible
- Blocking calls (recv, accept...) made asynchronous using callback
- Socket and Server for tcp, analogous to nodejs "net" (can be used for tls, http(s))
- UDP connectionless
- Linux only

## Todo

- UDP connect
- Nicer wrapper around Node and Network
- More features from ZeroTierSockets.h
- Configure macos and windows
