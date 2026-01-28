# Ray Tracer

## Overview

This is a Work In Progress Ray Tracing Engine. The goal is to create a
versitile engine capable of running in various environments including 
embedded systems*, distributed systems over TCP using various backends like
OpenCL, CUDA or raw CPU SIMD.

_\* I know it has no practical purpose, but I want to torture my STMs with
some number crunching :3_

## Architecture

The engine is separated into 3 major components:
 * The Tracer Core
 * Scene Loader / Preprocessor
 * CLI / Server

### The Core Tracer

It's not hard to guess what it does, it receives the scene to be rendered 
and... well - renders it.

### Scene Loader / Preprocessor

Scene Loader and Preprocessor is responsible for loading the scene from GLTF
files (_in the future possibly FBX files as well_), doing the preprocessing 
like BVH generation and preparing the scene for the Core to render.

### CLI / Server

This is the wrapper that tells the other components what to do. It can run in
two modes, either a CLI tool that takes a scene, a config and a render call
and performs the operation or as a server. In server mode it listens on a TCP
socket and receives the commands through that way. 

## What's working so far

 * Basic rendering
 * Point lights
 * Basic PBR BRDF (subset of Disney BRDF)

