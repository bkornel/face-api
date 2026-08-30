# 3DDFA_V2 model files

The shape model's default fitter regresses 62 3DMM parameters with the MobileNet-V1
backbone of 3DDFA_V2 (Guo et al., *Towards Fast, Accurate and Stable 3D Dense Face
Alignment*, ECCV 2020) and reconstructs the 68-landmark subset of the Basel Face Model
from the basis data next to it.

| file | content | source |
|---|---|---|
| `mb1_120x120.onnx` | MobileNet-V1, 120x120 BGR in, 62 params out | huggingface.co/Stable-Human/3ddfa_v2 |
| `param_mean.bin`, `param_std.bin` | 62 float32, parameter de-normalization | huggingface.co/litert-community/3DDFA-V2-LiteRT |
| `u_base.bin` | 204 float32, mean shape (x,y,z per landmark) | huggingface.co/litert-community/3DDFA-V2-LiteRT |
| `w_shp_base.bin` | 204x40 float32, shape basis | huggingface.co/litert-community/3DDFA-V2-LiteRT |
| `w_exp_base.bin` | 204x10 float32, expression basis | huggingface.co/litert-community/3DDFA-V2-LiteRT |

Everything is little-endian float32. Upstream project and weights are MIT licensed
(github.com/cleardusk/3DDFA_V2).

`FaceModel::mShape3D` is derived from `u_base.bin` as well, so the head that
`solvePnP` fits is the same one the network regresses. `Testing/tools/GenerateFaceModel.cpp`
performs that derivation - y and z flipped to image orientation, origin at the nose tip,
scaled to a 54.56 mm rms radius - and prints the table ready to paste.
