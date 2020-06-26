# How to publish / setup apt-get repo for kernel modules

### Prepare a GPG key

```
# Setup GPG key to use, export them and install on target system
gpg --generate-key
# Export public key
gpg --export --armor >public_key.pub
# Import on target system
apt-key add public_key.pub
```

### Setup aptly with config similar to:

```
{
  "rootDir": "$HOME/.aptly",
  "downloadConcurrency": 4,
  "downloadSpeedLimit": 0,
  "architectures": [],
  "dependencyFollowSuggests": false,
  "dependencyFollowRecommends": false,
  "dependencyFollowAllVariants": false,
  "dependencyFollowSource": false,
  "dependencyVerboseResolve": false,
  "gpgDisableSign": false,
  "gpgDisableVerify": false,
  "gpgProvider": "gpg",
  "downloadSourcePackages": false,
  "skipLegacyPool": true,
  "ppaDistributorID": "ubuntu",
  "ppaCodename": "",
  "skipContentsPublishing": false,
  "FileSystemPublishEndpoints": {},
  "S3PublishEndpoints": {
    "apt_lohmega": {
      "region": "eu-west-2",
      "bucket": "apt.lohmega.com",
      "awsAccessKeyID": "-------",
      "awsSecretAccessKey": "-------",
      "acl": "public-read"
      }
  },
  "SwiftPublishEndpoints": {}
}
```

```
# Create and publish local repo if it doesn't exist
aptly repo create lohmega
aptly publish repo -distribution=testing lohmega s3:apt_lohmega:
```

```
# Build kernel modules on a pi with the correct os
KERNEL_SRC=/lib/modules/$(uname -r)/build ARCH=arm make -f Makefile.kernel modules
# Create deb package
./porting/dpl/kernel/bin/create-debian-rpi-pkg.sh CREATE uwbcore 0.1-3

# Add to repo
aptly repo add lohmega ./uwbcore-4.19.97-v7+_0.1-3_armhf.deb
# Update published repo
aptly publish update testing s3:apt_lohmega:
```