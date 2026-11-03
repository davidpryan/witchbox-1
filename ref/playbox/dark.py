"""What the Witchbox toys (pages three and four) share.

They read the chords from one place, settings.CHORDS, the same one the band
uses, so any of them fit together. Their strip shows Scale, Chords, Tempo
and Volume (no Sound: each of these toys is its own instrument). The Scale
button and Random keep to the dark scales.

    self.root(ahead=0)    the current chord's root, as a scale step 0..6
    self.bar_no()         bars since Play (the band's count, in the band)
    self.kick()           a drum toy calls this for every kick, so pads can duck
    self.new_kick()       True once for each kick heard since the last call
"""
import settings
from settings import BAR
from toy import Toy


class Dark(Toy):
    SHARED = (settings.SCALE, settings.CHORDS, settings.TEMPO, settings.VOLUME)
    CELLS = ((106, 66), (172, 60), (232, 34), (266, 46))
    SCALE_SET = settings.DARK_SCALES
    _kicks = 0

    def enter(self):
        if settings.SCALE.value not in self.SCALE_SET and not settings.band:
            settings.SCALE.value = self.SCALE_SET[0]        # start in a dark scale
        Toy.enter(self)

    def root(self, ahead=0):
        return settings.CHORDS.root(self.steps, ahead) % 7

    def now(self):
        """The clock step everyone agrees on: the band's, or this toy's own."""
        return settings.band_steps if settings.band else self.steps

    def bar_no(self):
        return self.now() // BAR

    def kick(self):
        self.note(0, 0)

    def new_kick(self):
        if settings.kicks != self._kicks:
            self._kicks = settings.kicks
            return True
        return False
