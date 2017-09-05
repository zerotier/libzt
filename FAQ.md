## Frequently Asked Questions
***

### Can I use this in my commercial or closed-source application or service?

Yes! - Just let us know, and we will work out a licensing scheme. You will need to build the library with the `lwIP` network stack. In the case that you're a non-profit, or are developing open source software, you can use this entirely for free and may choose either `picoTCP` or `lwIP` as your network stack.

### Application or service won't fully come online

Sometimes it can take a substatial amount of time for libzt to come online and become reachable. In cases where it never seems to progress beyond this stage you should check to make sure there are no rogue processes on the machines using the same ZeroTier identity files, or no other instances on your ZeroTier network using said identity files. If this doesn't help. Contact us.