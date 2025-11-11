# RayTracer

About 10th attempt at this project T-T

Well... Here we go again. The **biggest** obstacle whenever I attempted this project was scope creep. That's why here I lay out the general idea of what I want it to be.

## General Project Architecture

### Scene Preparation

We'll use unity for it. It's simple, supports a lot of image and mesh formats. We'll create a simple script that takes the scene. Converts the meshes, materials and textures to the usable format, stores them as binary blobs, JSONs and PNGs, and compresses it to some kind of archive. This part unfortunatelly has to be done in C# (_bleh_).

#### Stored Formats

##### Meshes
Stored as a binary blob (.bin file) + metadata (.json file). The structure of each vertex will be:
```c
struct Vertex
{
    float pos[3];        // Always present
    float normal[3];     // Always present
    float tex_coord[2];  // Optional: Only for textured meshes
    float tangent[3];    // Optional: Only for normal mapped meshes
}

struct Triangle
{
    Vertex verts[3];
}
```

##### Materials
Stored as .json files, with following structure:
```c
struct TexRef
{
    int tex_index;    //
    float offset[2];  //
    float scaling[2]; //
}

// Pointers may end up being indices into some texture metadata array.
struct Material
{
    float color[4];   // Alpha dictates the transparency
    TexRef *basemap;  // 
    float metal;      //
    float rough;      //
    float occlusion;  // Shouldn't be used but here for completeness
    float ior;        // Added manually
    TexRef *mromap;   //  
    TexRef *normmap;  //
    float normstr;    //
    TexRef *detmask;  //
    TexRef *detalb;   //
    TexRef *detnorm;  //
    float emi[3];     // 
    TexRef *emimap;   //
}
```

##### Textures
Also stored as .json files with, following structure:
```c
struct Tex
{
    uint16_t w, h;   //
    float *pixels;   // } One of those formats will be used.
    hdrc *pixels;    // } HDRC refers to common exponent 5999 format.
    uint8_t *pixels; // }
    uint8_t flags;   // Wrap and filter settings.
}
```

##### Lights
Also stored as .json files with, following structure:
```c
struct Light
{
    uint8_t type;   // Point, Area, Directional, Spot
    float color[3]; // 
    float pos[3];   // For all except Directional
    float dir[3];   // For all except Point
    float size[2];  // [2] - for Area, [1] for the rest
    float angle;    // For Spot only
}

// Skylight will get it's own structure 
struct SkyLight
{
    TexRef tex;
    float exposure;
    float gamma;
    float rotation;
}
```

##### Camera
Also stored as .json files with, following structure:
```c
struct Camera
{
    float pos[3];  //
    float dir[3];  //
    float fov;     //
    float near;    //
    float far;     //
    float focal;   // } Added manually
    float aper;    // } 
}
```

##### Textures
Stored as .png or .hdr, depending on the type, with .json metadata with following structure:
```c
struct Texture
{
    char *filename;  // 
    uint8_t flags;   // HDR?, filter, wrap
};
```

### Frontend - Scene load

This will either be done in C or Python. This part will be responsible for decompressing the archive and loading the data into structures. Should be simple enough to implement. This will not be a part of the stripped down version for low-resource devices.

### Midend - Optimization

Also done in C or Python. This will take the scene and generate the KD trees for the meshes and BVH for the whole scene.

### Backend - The _Sweet Sweet_ Algorithm

Initially written in C, later may be ported to some GPGPU platform like OpenCL.

## Planned Features

