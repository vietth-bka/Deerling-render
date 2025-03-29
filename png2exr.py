from PIL import Image
import numpy as np
import OpenEXR
import Imath

# Load the PNG image
img = Image.open("tests/textures/black.png")
img_np = np.array(img) / 255.0  # Normalize to [0, 1]

# Extract R, G, B channels
height, width, _ = img_np.shape
r = img_np[:, :, 0].astype(np.float32)
g = img_np[:, :, 1].astype(np.float32)
b = img_np[:, :, 2].astype(np.float32)

# Create EXR file
exr_path = "tests/textures/black.exr"
header = OpenEXR.Header(width, height)
header['channels'] = {
    'R': Imath.Channel(Imath.PixelType(Imath.PixelType.FLOAT)),
    'G': Imath.Channel(Imath.PixelType(Imath.PixelType.FLOAT)),
    'B': Imath.Channel(Imath.PixelType(Imath.PixelType.FLOAT))
}

exr = OpenEXR.OutputFile(exr_path, header)
exr.writePixels({'R': r.tobytes(), 'G': g.tobytes(), 'B': b.tobytes()})
exr.close()

print(f"EXR file saved as {exr_path}")