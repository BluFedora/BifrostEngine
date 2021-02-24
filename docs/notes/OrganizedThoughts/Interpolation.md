# Interpolation

#math

## Implicit Equations

These are equations that when they hold true then it is included in the line.
For example: $$Y = X$$ is a straight line with a slow of 1 that passes through the origin.
$$Circle: X^2 + Y^2 = Radius^2$$.

## Parametric Equations

Have one input parameter that everything is based on. This allows for turning a single number into a more complex datatype such as a Vector, Color, or Quaternion.

The parametric version of $$Y = X$$ is $$P(t) = \{t, t\}$$
$$Circle(t) = Radius * \{cos(2\pi * t), sin(2\pi * t)\}$$

## Bezier Curves

A (Quadratic) bezier curve with three point `A`, `B`, and `C` would be at `A` when t = 0.0, and C when t = 1.0 but near the middle would be weighted towards B. (A blend of two Linear Bezier Curves)

$$E(t) = lerp(A, B, t)$$ 
$$F(t) = lerp(B, C, t)$$ 
$$BQuadratic(t) = lerp(E(t), F(t), t)$$

A Cubic Bezier curve is 6 lerps.

![[BezierCuverEquationsSimplified.png]]

## Splines

A list of curves connected at the ends.

### Hermite

Instead of B and C guide points  you give a velocity a each point.
Ensuring continuity in a Hermite spline the end of one curve's velocity should match the velocity of the next curve beginning.

$$CubicHermite(t) = s^2(1 + 2t)A + t^2(1+2s)D + s^2tU+st^2V$$

```c
// Hermite To Bezier
B = A + (U / 3)
C = D - (V / 3)

// Bezier To Hermite
U = 3 * (B - A)
V = 3 * (D - C)
```

### Catmull-Rom

An algorithm for calculating a Hermite spline.

1. Start with a Set of Points
2. Assume the last and end point's velocities are 0.
3. Start At `index = 0`.
4. While `index < spline.size - 2`
  1. Grab the next two points (`P[index + 1]` and `P[index + 2]`)
  2. Assign `P[index + 1].velocity = (P[index + 2] - P[index + 0]) / 2`
  3. `++index`
  4. Go Back To Step 4.

### Cardinal

Same as Catmull-Rom but adds a `Tension` [0.0 -> 1.0] parameter, multiply each velocity by `(1.0 - Tension)`.

`Tension == 0.0` is equal to catmull-rom.

`Tension == 1.0` causes velocities to be 0 so the splines is just a set of straight lines.

### Kochanek–Bartels
TODO

### B-Splines
TODO

## Curved Surfaces
TODO

## References

[[interpolation-and-splines.pdf]]
