import sys
import numpy as np
import cv2
from . import cdither
from typing import TypeVar, Iterable, Union


def dither(image, palette: Union[str, Iterable]='bw', out='img', method='fs'):

    if isinstance(palette, str):
        if palette in palettes:
            palette = palettes[palette]
        else:
            raise Exception('Unrecognized palette name \'%s\'. Supported palettes: %s.' %
                            (palette, ', '.join(palettes)))

    
    if method == 'fs':
        dithered, indexes = cdither.floyd_steinberg(image, np.asarray(palette, dtype='float32'))
    elif method == 'closest':
        dithered, indexes = cdither.closest(image, np.asarray(palette, dtype='float32'))
    

    dithered = np.asarray(dithered).astype(np.uint8)

    if out == 'img':
        return dithered
    elif out == 'ids':
        return indexes
    elif out == 'both':
        return dithered, indexes
    else:
        raise Exception('Invalid out argument: %s.' % out)


palettes = {
    'bw': [[0.,0.,0.], [255., 255., 255.]],
    
    'factorio-black': [[  0.,   0., 255.],  # red
                       [  0., 255.,   0.],  # green
                       [255.,   0.,   0.],  # blue
                       [  0., 255., 255.],  # yellow
                       [255.,   0., 255.],  # magenta
                       [255., 255.,   0.],  # cyan
                       [255., 255., 255.],  # white
                       [  0.,   0.,   0.]], # black

    'win16':   [[  0.,   0.,   0.], # black	
                [  0.,   0., 128.], # maroon	
                [  0., 128.,   0.], # green	
                [  0., 128., 128.], # olive	
                [128.,   0.,   0.], # navy	
                [128.,   0., 128.], # purple	
                [128., 128.,   0.], # teal	
                [192., 192., 192.], # silver	
                [128., 128., 128.], # gray
                [  0.,   0., 255.], # red
                [  0., 255.,   0.], # lime
                [  0., 255., 255.], # yellow
                [255.,   0.,   0.], # blue
                [255.,   0., 255.], # fuchsia
                [255., 255.,   0.], # aqua
                [255., 255., 255.]], # white
}
