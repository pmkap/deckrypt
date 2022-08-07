# Deckrypt
Unlock a LUKS-encrypted root partition with game controller combinations on Arch Linux.

This is meant for people who want full disk encryption for devices like the Steam Deck.

It consist of a C prgram that translates game controller combinations to a character sequence and then induces events as if the sequence was typed on a real keyboard. This sequence can then be added to a LUKS slot.

An initcpio hook provides this in early userspace to unlock root.

The advantage of this approach is that you can either use a controller combination to unlock the device **or** a regular password/keyboard.

## Combinations
When speaking of a button below, this also includes a direction with the D-pad or an analog stick. The four directions of these are simply translated to four "buttons".

A combination consists of
* any number of hold-buttons and
* one tap-button.

A button becomes a hold-button if it is pressed for at least a 250 ms. Hold-buttons have to be pressed before the tap-button to be part of the combination.

Combinations are separated by semicolons and thus build a sequence.

### Security
Let's take  a standard game contoller like the one from the PS2 or the Steam Deck let's only use the thumbs and index fingers. Each of those fingers is restricted to their own set of reachable controls.

| Finger |Controls |
| --- | --- |
| Left Thumb | D-pad, Left Analog Stick |
| Right Thumb | X/Y/A/B, Right Analog Stick |
| Left Index | L1/L2 |
| Right Index | R1/R2 |

As mentioned above, a combination consists of any number of buttons held plus one final button to conclude the combination. Let's break down for every finger the number of buttons it can hold or tap.

The left index finger for example can tap either L1 or L2 and hold either L1, L2 or no button at all.

The thumbs are a bit more complicated because of the D-pad/analog sticks. These are translated to four "buttons" each, which can be tapped. However, in terms of holding there are not just four "buttons" but rather eight directions.

So the right thumb can tap the four buttons X/Y/A/B, four directions with the analog stick or clicking the analog stick itself, resulting in nine enumerations. It can hold either of X/Y/A/B, eight directions, clicking the analog stick or none, which makes 14.

| Finger | Hold Enumerations | Tap Enumerations |
| --- | --- | --- |
| Left Thumb | 18 | 9 |
| Right Thumb | 14 | 9 |
| Left Index | 3 | 2 |
| Right Index | 3 | 2 |

Take the above table as a matrix $A$. The overall number of possible combinations equals to

$$\sum_{i=1}^{n}\frac{p}{A_{i,1}}A_{i,2}=5616$$

with

$$p=\prod_{i=1}^{n}A_{i,1}$$

Choosing four random combinations provides an entropy of 50 bits, choosing five gets you 62 bits of entropy.

## Preparation
Some things are hardcoded in `deckrypt_input.c` and need to be changed according to your controller and setup:
* buttons that can be used for combinations
* button to confirm
* button to clear

These will be matched with the Steam Deck once I get one (or a pull resquest).

`evtest` (available in the official repository) can be used to find a device and available buttons.

It is highly recommended to setup LUKS with a normal password and then use deckrypt with another slot. See [cryptsetup(8)](https://man.archlinux.org/man/cryptsetup.8.en).

## Dependencies
* libevdev (official repos)
* [libkeymap](https://aur.archlinux.org/packages/libkeymap) (AUR)

## Installation
* Compile the `deckrypt_input` binary with `make` and make it available in your path.
* Install the initcpio hooks.
* Add `deckrypt` (directly before `encrypt`) in the `HOOKS`-line in `/etc/mkinitcpio.conf` and regenerate initramfs.

(I will make an AUR package for these steps sometime.)
