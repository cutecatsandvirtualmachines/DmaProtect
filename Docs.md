# Protect your code from DMA

This repo is designed as an example of how to set up a simple VT-d/IOMMU configuration capable of protecting physical pages
from r/w operations.

There's not many resources on DMA protection online, especially non-UEFI code is absent for both Intel and AMD.

[Tandasat's example](https://github.com/tandasat/HelloIommuPkg) shows how to set up VT-d on boot, and I pasted most of the intel code from there.

AMD examples don't exist, therefore refer back to their documentation, which is included in this project (along with Intel's too) since it's
quite literally **bordeline impossible to even get your hands on** the latest revision of the document, as AMD will remove links to their "old" files.

## How it works

On an extremely simple level, both VT-d and IOMMU perform as their major function something very similar to Intel's [EPT](https://en.wikipedia.org/wiki/Second_Level_Address_Translation).

By following the documentation you're able to setup a new set of page tables that translate from the DMA device's physical address, to an
host physical address of choice.

In this example I'm basically redirecting r/w operations to a dummy page, which results in no error from a DMA device perspective while
still protecting the memory region.

## Requirements
On some Intel machines, no matter the windows version, I have noticed that Windows will freeze up due to some command polling that gets
stuck for VT-d, probably due to my modifications to the base configuration.

A possible workaround for that is [disabling Memory Integrity](https://www.makeuseof.com/core-isolation-memory-integrity-windows/)
and going through all services in Computer\HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services, and look for a value DmaRemappingCompatible under key Parameters.

[When the value is anything but 0](https://learn.microsoft.com/en-us/windows-hardware/drivers/pci/enabling-dma-remapping-for-device-drivers), it can [cause issues](https://forum.rme-audio.de/viewtopic.php?id=30110).

AMD doesn't seem to suffer from this same issue and can coexist on all windows versions with Windows.

### Usermode

This protection can obviously be used to protect usermode addresses too, as long as one is fully aware of what he's doing.

Particularly memory locking is **required** as otherwise physical addresses will change, resulting in probable MEMORY_MANAGMENT BSODs.

## Implementation problems

The implementation as it is in this repo is fairly easy to bypass by a DMA capable device, since the page tables don't protect themselves.

Implementing a fix for it is trivial if you know what you're doing, therefore it won't be included to avoid pasting.

# Contacts

If you're interested in hiring me contact on my brand new email:

cutecatsandvirtualmachines@gmail.com
