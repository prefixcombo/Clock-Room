# Clock Room

A watchface for Pebble devices that is based on the clock room in Castlevania: Symphony of the Night. I created the assets in GIMP using sprite sheet background rips from https://www.spriters-resource.com/playstation/cvsotn/. To make them actually look decent on the watches, I then converted the images to the supported 64 color palette with Floyd-Steinberg reduced color bleeding dithering.

Get it on the Pebble app store here: https://apps.repebble.com/35dd38e09ea647c181dd1b35

Only for Pebbles with color support:

- Basalt (Time/Time Steel)
- Chalk (Time Round)
- Emery (Time 2)
- Gabbro (Round 2)

I did attempt to add support for the B/W Pebbles but I used RotBitmapLayers to rotate the clock hands which don't work well on the B/W devices.
