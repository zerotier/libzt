# Nodejs libzt binding

## Building

### Build

```bash
cd src/bindings/nodejs
npm ci                  (rerun after changes to native code)
npm run build           (rerun after changes to typescript)
npm test (TODO)
```

### Build Debug

```bash
cd src/bindings/nodejs
npm ci --ignore-scripts
npm run compile-debug   (rerun after changes to native code)
npm run build-debug     (rerun after changes to typescript)
npm test (TODO)
```

## Running

Test source code can be found in `./src/test/`.
Run with no arguments to get description.

```bash
node dist/test/newudptest.js
node dist/test/newtcptest.js
```

## Current features

- Typescript
- Subset of ZeroTierSockets.h directly accessible
- Blocking calls (recv, accept...) made asynchronous using callback
- Socket and Server for tcp, analogous to nodejs "net" (can be used for tls, http(s))
- UDP
- Linux only

## Todo

- UDP flags
- convert TCP to lwip callback api
- Nicer wrapper around Node and Network
- More features from ZeroTierSockets.h
- Configure macos and windows (might work already, not tested)
