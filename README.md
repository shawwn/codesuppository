# John Ratcliff's codesuppository

## Configurable approximate convex decomposition of non-closed meshes

![image](https://user-images.githubusercontent.com/59632/58806604-2b768e00-85dc-11e9-88da-5020422c25bd.png)

### How does it work?

Here's an [explanation of the algorithm.](https://web.archive.org/web/20080613123719/http://codesuppository.blogspot.com/2006/04/approximate-convex-decomposition.html)

### Example?

Input mesh:

![image](https://user-images.githubusercontent.com/59632/58805963-c8382c00-85da-11e9-8edb-52b3c36a75e4.png)

Decomposed into a set of convex hulls:

![image](https://user-images.githubusercontent.com/59632/58806066-f87fca80-85da-11e9-8682-f5a3658444a7.png)

Notably, this algorithm does not use voxels. Therefore it may be more efficient than algorithms like [HACD](https://github.com/kmammou/v-hacd).

Hulls usually take a few seconds to generate regardless of the complexity of the input mesh. (The hulls in the screenshot above took around 10 seconds to generate.)
