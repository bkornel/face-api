# Head model

The head the avatar is built on. It is an authored model rather than a generated one,
because sixty-eight landmarks cannot describe a head: nothing built out of them alone has
ears, a neck, or an eyelid that folds. What they *can* do is move one that already has them.

| file | content | source |
|---|---|---|
| `ict_neutral_head.obj` | 26719 vertices, quads, with a face, a back of head, ears, a neck stub, a mouth socket and two eyeballs | ICT-FaceKit `FaceXModel/generic_neutral_mesh.obj` |
| `LICENSE.txt` | The MIT licence the model is published under | ICT-FaceKit `LICENSE` |

Upstream: https://github.com/USC-ICT/ICT-FaceKit, commit `aa8c2417` (2020-07-23).
Copyright (c) 2020 USC Institute for Creative Technologies, MIT licensed.

## What was changed

The file here is trimmed, and the header inside it says so as well:

- The texture coordinates are gone; nothing textures this model.
- The faces of the parts the avatar never shows are gone: teeth, gums and tongue, lacrimal
  fluid, the eye blend and occlusion shells, and the eyelashes.
- The neck is cut half an inter-ocular span below the chin. Upstream it continues into a
  collar that flares wider than the head itself, which frames badly in a panel and drags the
  camera back off the face.

**Vertex numbering is unchanged.** That is the one thing that had to survive: the ICT-FaceKit
README publishes the vertex indices of the 68 Multi-PIE landmarks on this topology, and
[`HeadMesh`](../../../../FaceApi/Model/HeadMesh.h) reads them from that table. Without them
the landmarks would have to be guessed by proximity, and a guess at the eye corners is a
squint. The vertices of the removed parts are therefore still in the file, unreferenced;
`HeadMesh::Compact` drops them once the indices have been read.

## Why this model

Its landmark layout is the same Multi-PIE 68 the pipeline itself speaks, so the binding
between the tracked face and the mesh is exact rather than approximate. Measured against the
pipeline's own canonical face, the two agree to a mean of 0.08 inter-ocular spans over every
pair of landmarks — the residual being that they are simply two different faces, which is the
difference the avatar exists to show.
