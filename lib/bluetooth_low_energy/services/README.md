## Moble SDK Version Compatibility

This table denotes the version comapatibility between BLE device code and mobile SDKs. Each row in the table contains a master version number, commit ID of the device code and their corresponding compatible version tags of IOS and Android mobile SDKS.
Version tags for IOS and Android mobile SDK versions are prefixed with branch name, followed by the version number.

* Verison number format is as follows: `<Major Version>.<Minor Version>`
* For every new release of either SDKs or device code, the minor versions of the corresponding repositories are incremeted.
* Major version is incremented only when there is a backwards incompatible change in protocol between the device and mobile SDKS. Major versions of both device and Android SDKs should be upgraded for a backwards incompatible change.
* Combination represented by each row in the table is tested and guarenteed to be compatible.
* Combinations between different minor versions are not tested.
* Combinations between different major versions should never be used.


|  Master Version    |  Device Commit ID                          |  IOS SDK Version Tag |  Android SDK Version Tag   |
|:------------------:|:------------------------------------------:|:--------------------:|:--------------------------:|
|   Pre-release-0.1  | b3779c0fef978e04e6ade33ce3a10ad953fdbd7d   | Beta_0.1             | Beta-demo_0.1              |
|                    |                                            |                      |                            |
