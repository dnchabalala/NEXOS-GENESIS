# NexOS TLS trust store

`cacert.pem` is the Mozilla CA bundle published by curl.se, downloaded from
`https://curl.se/ca/cacert.pem` on 2026-10-03.

SHA-256:

```text
a41b5d356aea97a529fe27e0f7316d2f9d946d75927476cf9cf1b90637d00505
```

The bundle contains genuine CA certificates and is passed to mbed TLS's
X.509 chain validator. Hostname verification and validity-period checks stay
enabled; no site certificate is pinned and no trust-all mode is used.
