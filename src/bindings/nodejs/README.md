# Nodejs libzt binding

## Current features
- Typescript
- Subset of ZeroTierSockets.h directly accessible
- Blocking calls (recv, accept...) made asynchronous using callback
- Socket and Server for tcp, analogous to nodejs "net" (can be used for tls, http(s))
- Linux only

## Todo
- UDP
- Nicer wrapper around Node and Network
- More features from ZeroTierSockets.h
- Configure macos and windows
