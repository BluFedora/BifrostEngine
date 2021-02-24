# Mathematics

#math

you can skip transpose/inverse part of your model matrix does not do non-uniform scaling

## Lerp / Smoothing

### Framerate Independence

[Reference](https://gamasutra.com/blogs/ScottLembcke/20180404/316046/Improved_Lerp_Smoothing.php)

A moving lerp can be used to smoothly get to a target value using `value = lerp(value, target_value, /* coefficient: usually a low number like 0.05 - 0.2 */)`.

*This is NOT framerate independent unless you use a fixed time step which isn't always desirable.*

To fix this an exponential function involving time can be used instead:
`value = lerp(target_value, value, exp[base](-rate * delta_time))`

- A `rate` of 1.0 means `value` will reach 1.0 / `base` of `target_value` in one second.
- Halving and doubling `rate` will change how fast it moves as expected.

To convert your old `coefficient` to the new rate:

`rate = -target_fps * log[base](1.0 - coefficient)`

To make a more slider friendly number to edit just have a `log_rate` variable that is edited and to get the `rate` do `exp[base](log_rate)`.

### Logarithmic Lerping

Some values are better moving on a logarithmically namely "scale / zoom".

```cpp
zoom = lerp(zoom, target_zoom, delta_time / duration);
=>
zoom = exp[base](lerp(log(zoom), log[base](target_zoom), delta_time / duration);
```

## Linear Algebra

### Vector

![[CrossProductDerivation.png]]

```glsl
// Dot Product
dot(A, B) = |A||B|cos(theta)
// aside: cross(A, B) = |A||B|sin(theta)

// Projection
proj_a(b) = (dot(a, b) / lensq(a)) * a

// Rejection (Up from Projection)
reject_a(b) = a - proj_a(b)

    /|
  a/ | <- rejection
  /  |      b
 --------------
      ^projection
      
// Reflection

relfection(a, n) = a - (2 * dot(a, n) / dot(n, n)) * n
```

### Quaternion

```glsl
A quaternion represents an expression: 
q = w + i*x + j*y + k*z

i^2 = j^2 = k^2 = i*j*k = -1

A `pure` quaternion is one in which the w component is 0.0f. 

p_1 = q * p_o * q^

Notation: q^ = q's conjugate (aka negate x, y, and z members)

// Axis Angle

Axis  = normalize(q.xyz);
Angle = 2.0 * acos(q.w) 
```

### Matrix

```glsl
R = A^T
=> transpose(inverse(R)) = inverse(A) = transpose(inverse(transpose(A)))
```

#### Normal Mapping

> The normal of an object must be transformed by the transposed inverse of the model to world matrix.

```glsl
mat4 NormalMatrix      = transpose(inverse(ModelMatrix));
vec4 TransformedNormal = NormalMatrix * ObjectNormal;
```
> The TBN matrix can be calculated in this way:

```glsl
vertex-inputs:
  vec3 in_Tangent;
  vec3 in_Normal;

// Gram–Schmidt Orthonormalization

vec3 T   = normalize(mat3(NormalMatrix) * in_Tangent);
vec3 N   = normalize(mat3(NormalMatrix) * in_Normal);
T        = normalize(T - dot(T, N) * N);
vec3 B   = cross(N, T);
mat3 TBN = transpose(mat3(T, B, N));
```

#### Decomposition

```glsl
// Row Major Matrix Representation.

mat4 M = [a, b, c, d]
         [e, f, g, h] 
         [i, j, k, l] 
         [0, 0, 0, 1]

vec3 Translation = vec3{d h l};
vec3 Scale       = [
  length([a, e, i]),
  length([b, f, j]),
  length([c, g, k]),
];
mat4 Rotation = [a / Scale.x, b / Scale.y, c / Scale.z, 0]
                [e / Scale.x, f / Scale.y, g / Scale.z, 0]
                [i / Scale.x, j / Scale.y, k / Scale.z, 0]
                [      0,           0,           0,     1]
```
