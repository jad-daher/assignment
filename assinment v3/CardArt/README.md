# Card art generator (cartoon portraits)

Regenerates `GLProject/textures/card_1.jpg` ... `card_4.jpg` from the photos in `photos/`.

    pip install opencv-python pillow numpy
    python make_cards.py            # writes straight into ../GLProject/textures

* Change the printed names / subtitles / colours in the `NAMES` dict at the top of `make_cards.py`.
* Card order = `CARD_ORDER` (p1..p5 -> card_1..card_5).
* `HEAD` in `cartoon.py` holds each head position in the original photo (centre x,y + half-width/height in px);
  nudge those numbers if a face is cropped badly. `ZOOM` controls how big the face is inside the medallion.
