ZeroTier SDK (beta)
======

ZeroTier-enabled apps. Virtual network access embedded directly into applications and games.

## What does it do?

Imagine starting an instance of your application or game and having it automatically be a member of your virtual network without having to rewrite your networking layer.

## How is it used?

There are generally two ways one might want to use this SDK/service. 

 - The first approach is a *compile-time static linking* of our SDK/service directly into your application. With this option you can bundle our entire functionality right into your app with no need to communicate with a service externally, it'll all be handled automatically. 

 - The second is a service-oriented approach where our SDK is *dynamically-linked* into your applications upon startup and will communicate to a single ZeroTier service on the host. This can be useful if you've already compiled your applications and can't perform a static linking.

![Image](docs/img/methods.png)

## How does it work?

We've built a special background service that pairs the ZeroTier protocol with a user-space [Lightweight IP (lwIP) stack](http://savannah.nongnu.org/projects/lwip/) to create a new way for you to bring your applications onto your virtual network. Check out our [Integrations](integrations/) to learn how to integrate this with your particular application.

For a more in-depth explanation of our technology take a look at our [SDK Primer](docs/zt_sdk.md)