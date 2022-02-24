# Deckrypt
Unlock a LUKS-encrypted root partition with game controller combinations on Arch Linux.

This is meant for people who want full disk encryption for devices like the Steam Deck.

It consist of a C prgram that translates game controller combinations to a character sequence and then induces events as if the sequence was typed on a real keyboard. An initcpio hook provides this in early userspace.

## Combinations
A combination consists of
* any number of hold-buttons and
* one tap-button.

A button becomes a hold-button if it is pressed for at least a 250 ms. Hold-buttons have to be pressed before the tap-button to be part of the combination.

Combinations are separated by semicolons and thus build a sequence.

### Security
Let's assume we have ten buttons available for combinations and we use up to four buttons for a single combination. The Number of possible combinations equals to

![equation_no_of combinations.png](https://user-images.githubusercontent.com/48184470/154338092-4494df71-c916-49b5-8422-7626607abf99.png)

Choosing five random combinations provides an entropy of 51 bits, choosing six gets you 62 bits of entropy.

This is not entirely correct because some buttons have to be pressed by the same finger, reducing the number of possibilies. But the Steam Deck has a lot more buttons available so this should be seen as an estimate.

## Preparation
Some things are hardcoded in `deckrypt_input.c` and need to be changed according to your controller and setup:
* buttons that can be used for combinations
* button to confirm
* button to abort

These will be matched with the Steam Deck once I get one (or a pull resquest).

`evtest` (available in the official repository) can be used to find a device and available buttons.

## Dependencies
* libevdev (official repos)
* libkeymap (AUR)

## Installation
* Compile the `deckrypt_input` binary with `make` and make it available in your path.
* Install the initcpio hooks.
* Add `deckrypt` (directly before `encrypt`) in the `HOOKS`-line in `/etc/mkinitcpio.conf` and regenerate initramfs.

(I will make an AUR package for these steps sometime.)
