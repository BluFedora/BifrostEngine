mergeInto(LibraryManager.library,
{
  basicDrawing: function(gl) {
    console.log(gl);

    gl = document.getElementById("kanvas").getContext("webgl2");

    if (gl)
    {
      var vertexShaderSource = `#version 300 es
      precision mediump float;
      // an attribute is an input (in) to a vertex shader.
      // It will receive data from a buffer
      in vec4 a_position;

      // all shaders have a main function
      void main() {

        // gl_Position is a special variable a vertex shader
        // is responsible for setting
        gl_Position = a_position;
      }
      `;

      var fragmentShaderSource = `#version 300 es
      precision mediump float;

      // we need to declare an output for the fragment shader
      out vec4 outColor;

      void main() {
        // Just set the output to a constant redish-purple
        outColor = vec4(1, 0, 0.5, 1);
      }
      `;

      function createShader(gl, type, source) {
        var shader = gl.createShader(type);
        gl.shaderSource(shader, source);
        gl.compileShader(shader);
        var success = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
        if (success) {
          return shader;
        }

        console.log(gl.getShaderInfoLog(shader));
        gl.deleteShader(shader);
      }

      var vertexShader = createShader(gl, gl.VERTEX_SHADER, vertexShaderSource);
      var fragmentShader = createShader(gl, gl.FRAGMENT_SHADER, fragmentShaderSource);

      function createProgram(gl, vertexShader, fragmentShader) {
        var program = gl.createProgram();
        gl.attachShader(program, vertexShader);
        gl.attachShader(program, fragmentShader);
        gl.linkProgram(program);
        var success = gl.getProgramParameter(program, gl.LINK_STATUS);
        if (success) {
          return program;
        }

        console.log(gl.getProgramInfoLog(program));
        gl.deleteProgram(program);
      }

      var program = createProgram(gl, vertexShader, fragmentShader);
      var positionAttributeLocation = gl.getAttribLocation(program, "a_position");
      var positionBuffer = gl.createBuffer();

      var positionData = new Float32Array(6);

      positionData[0] = 0.0;
      positionData[1] = 0.0;
      positionData[2] = 0.0;
      positionData[3] = 1.0;
      positionData[4] = 1.0;
      positionData[5] = 0.0;

      gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
      gl.bufferData(gl.ARRAY_BUFFER, positionData, gl.STATIC_DRAW);

      var vao = gl.createVertexArray();
      gl.bindVertexArray(vao);
      gl.enableVertexAttribArray(positionAttributeLocation);

      var size      = 2;        // 2 components per iteration
      var type      = gl.FLOAT; // the data is 32bit floats
      var normalize = false;    // don't normalize the data
      var stride    = 0;        // 0 = move forward size * sizeof(type) each iteration to get the next position
      var offset    = 0;        // start at the beginning of the buffer
      gl.vertexAttribPointer(
          positionAttributeLocation, size, type, normalize, stride, offset);

      var clear_color = 0.0;
      var clear_color_dir = 0.005;

      function resize(canvas)
      {
        var cssToRealPixels = window.devicePixelRatio || 1;
        var displayWidth    = (cssToRealPixels * canvas.clientWidth) | 0;
        var displayHeight   = (cssToRealPixels * canvas.clientHeight) | 0;

        if (canvas.width !== displayWidth) {
          canvas.width  = displayWidth;
        }

        if (canvas.height !== displayHeight) {
          canvas.height = displayHeight;
        }
      }

      function renderLoop()
      {
        resize(gl.canvas);

        // Tell WebGL how to convert from clip space to pixels
        gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);

        // Clear the canvas
        gl.clearColor(clear_color, clear_color, clear_color, 1);
        gl.clear(gl.COLOR_BUFFER_BIT);

        // Tell it to use our program (pair of shaders)
        gl.useProgram(program);

        // Bind the attribute/buffer set we want.
        gl.bindVertexArray(vao);

        Module._updateFromWebGL();
        clear_color += clear_color_dir;

        if (clear_color > 1.0 || clear_color < 0.0)
        {
          clear_color_dir = -clear_color_dir;
        }

        window.requestAnimationFrame(renderLoop);
      }

      renderLoop();
    }
  },
});