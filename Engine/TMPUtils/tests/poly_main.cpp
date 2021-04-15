#include "bf/bf_poly.hpp"

#include <cstdio>

using namespace bf;

struct Circle
{
  int pos[2];
  int radius;
};

struct Square
{
  int pos[2];
  int size[2];
};

struct SpecialShape
{
  // SpecialShape(const SpecialShape&) = delete;

  void draw(int color)
  {
    std::printf("drawing SpecialShape w/ color{%i}\n\n", color);
  }
};

void draw(Circle& my_circle, int color)
{
  std::printf("drawing circle @{%i, %i} with r{%i} w/ color{%i}\n\n", my_circle.pos[0], my_circle.pos[1], my_circle.radius, color);
}

void draw(Square& my_sqr, int color)
{
  std::printf("drawing Square @{%i, %i} with size @{%i, %i} w/ color{%i}\n\n", my_sqr.pos[0], my_sqr.pos[1], my_sqr.size[0], my_sqr.size[1], color);
}

using IDrawableDecl = poly::decl<
 poly::FnDef<void(poly::ErasedTag&, int)>>;

/*
template<>
constexpr poly::concept_map<IDrawableDecl, Circle> poly::remap<IDrawableDecl, Circle>{
 [](Circle* obj, int color) -> void { draw(*obj, color); },
};

template<>
constexpr auto poly::remap<IDrawableDecl, Square> = poly::makeConceptMap<IDrawableDecl, Square>(
 [](Square* obj, int color) -> void { draw(*obj, color); });
 */

template<typename T>
constexpr poly::concept_map<IDrawableDecl, T> poly::remap<IDrawableDecl, T> = poly::makeConceptMap<IDrawableDecl, T>(
 [](T* obj, int color) -> void { draw(*obj, color); });

template<>
constexpr auto poly::remap<IDrawableDecl, SpecialShape> = poly::makeConceptMap<IDrawableDecl, SpecialShape>(poly::defMember<&SpecialShape::draw>());

class IDrawable
{
 public:
  Poly<IDrawableDecl, poly::RefStorage> poly_;

  template<typename T>
  IDrawable(const T& obj) :
    poly_{obj}
  {
  }

  void draw(int color) const { poly_.invoke<0>(color); }
};

void drawStuff3(IDrawable drawable_copy)
{
  drawable_copy.draw(973);
}

void drawStuff2(IDrawable drawable_copy)
{
  drawable_copy.draw(8462);

  drawStuff3(std::move(drawable_copy));
}

void drawStuff(const IDrawable& drawable)
{
  drawable.draw(123456789);
  drawStuff2(drawable);
}

template<typename R, typename... Args>
using IFunction = poly::decl<poly::FnDef<R(/*const */ poly::ErasedTag&, Args...)>>;

template<typename T, typename R, typename... Args>
constexpr poly::concept_map<IFunction<R, Args...>, T> poly::remap<IFunction<R, Args...>, T> = poly::makeConceptMap<IFunction<R, Args...>, T>(poly::defMember<&T::operator()>());

template<typename F>
class Function; /* undefined */

template<typename R, typename... Args>
class Function<R(Args...)>
{
 private:
  Poly<IFunction<R, Args...>> m_Poly;

 public:
  template<typename T>
  Function(const T& obj) :
    m_Poly{obj}
  {
  }

  Function(const Function& rhs) = default;
  Function(Function&& rhs)      = default;
  Function& operator=(const Function& rhs) = default;
  Function& operator=(Function&& rhs) = default;
  ~Function()                         = default;

  R operator()(Args... args) const
  {
    return m_Poly.invoke<0>(std::forward<Args>(args)...);
  }
};

int main(int argc, char* argsv[])
{
  Circle circle0{{4, 6}, 21};
  Square square0{{102, 105}, {327, 437}};

  draw(circle0, 456);
  draw(square0, 123);

  drawStuff(circle0);
  drawStuff(SpecialShape{});
  drawStuff(square0);

#if 1
  Function<int(void)> lambda_holder = [i = 0]() mutable {
    std::printf("Called a lambda %i\n", i);
    ++i;
    return i;
  };
#else
  Function<void()> lambda_holder = []() {
    std::printf("Called a lambda\n");
  };
#endif

  auto value_sematics = lambda_holder;

  lambda_holder();
  lambda_holder();

  value_sematics();
  value_sematics();

  return 0;
}
