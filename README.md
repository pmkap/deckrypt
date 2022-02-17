# Deckrypt
Unlock a LUKS-encrypted root partition with game controller combinations in Arch Linux.

This is meant for people who want full disk encryption for devices like the Steam Deck.

It consist of a C prgram that translates input events to a sequence of button-combinations. And an initcpio hook which lets you use this sequence as a LUKS passphrase in early userspace.

The hook is just an alteration of the official `encrypt` hook and falls back to keyboard if getting the sequence fails or is aborted.

## Combinations
A combination consists of
* any number of hold-buttons and
* one tap-button.

A button becomes a hold-button if it is pressed for at least a 250 ms. Hold-buttons have to be pressed before the tap-button to be part of the combination.

Combinations are separated by semicolons and thus build a sequence.

You can try this out with `deckrypt_input`, which will print the combinations to stdout.

### Security
Let's assume we have ten buttons available for combinations and we use up to four buttons for a single combination. The Number of possible combinations equals to

![equation_no_of combinations.png](https://user-images.githubusercontent.com/48184470/154338092-4494df71-c916-49b5-8422-7626607abf99.png)


Choosing five random combinations provides an entropy of 51 bits, choosing six gets you 62 bits of entropy.

This is not entirely correct because some buttons have to be pressed by the same finger, reducing the number of possibilies. But the Steam Deck has a lot more buttons available so this should be seen as a (conservative) estimate.

## Preparation
Some things are hardcoded in `deckrypt_input.c` and need to be changed according to your controller and setup:
* buttons that can be used for combinations
* button to confirm
* button to abort

These will be matched with the Steam Deck once I get one (or a pull resquest).

`evtest` (available in the official repository) can be used to find a device and available buttons.

## Dependencies
* libevdev

## Installation
* Compile the `deckrypt_input` binary with `make` and make it available in your path.
* Install the initcpio hooks.

## Adding a LUKS key
Use the `deckrypt_add_luks` bash script to add a controller sequence as a key for a LUKS-encrypted device: `deckrypt_luks_add /dev/sda1`

## mkinitcpio hook
* Replace the `encrypt`-HOOK with `deckrypt` in `/etc/mkinitcpio.conf`.
* Regenerate initramfs with `mkinitcpio`.
