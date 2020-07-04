var bf_LibWebGL = {
  bfWebGLInitContext: bfWebGLInitContext,
  bfWebGL_getUniformBlockIndex: bfWebGL_getUniformBlockIndex,
  bfWebGL_uniformBlockBinding: bfWebGL_uniformBlockBinding,
  bfWebGL_bindBufferRange: bfWebGL_bindBufferRange,
  bfWebGL_handleResize: bfWebGL_handleResize
};

function bfWebGLInitContext() {
  this.bifrost = {
    gl: document.getElementById("canvas").getContext("webgl2"),
    idMappings: {},
    createObjectID: function (obj) {
      var id = this.currentID;
      this.idMappings[id] = obj;
      this.currentID += 1;

      return id;
    },
    getObjectFromID: function (id) {
      return this.idMappings[id];
    },
    removeObjectID: function (id) {
      this.idMappings[id] = null;
    }
  };
}

function bfWebGL_getUniformBlockIndex(program_id, name) {
  var gl = this.bifrost.gl;
  var program = this.GL.programs[program_id];
  var result = gl.getUniformBlockIndex(program, UTF8ToString(name));

  return result;
}

function bfWebGL_uniformBlockBinding(program_id, index, binding) {
  var gl = this.bifrost.gl;
  var program = this.GL.programs[program_id];

  gl.uniformBlockBinding(program, index, binding);
}

function bfWebGL_bindBufferRange(target, index, buffer_id, offset, size) {
  var gl = this.bifrost.gl;
  var buffer = this.GL.buffers[buffer_id];

  gl.bindBufferRange(target, index, buffer, offset, size);
}

// [https://webglfundamentals.org/webgl/lessons/webgl-resizing-the-canvas.html]
function bfWebGL_handleResize() {
  var gl = this.bifrost.gl;
  var realToCSSPixels = window.devicePixelRatio;
  var displayWidth = Math.floor(gl.canvas.clientWidth * realToCSSPixels);
  var displayHeight = Math.floor(gl.canvas.clientHeight * realToCSSPixels);

  if (gl.canvas.width !== displayWidth || gl.canvas.height !== displayHeight) {
    gl.canvas.width = displayWidth;
    gl.canvas.height = displayHeight;
  }
}

mergeInto(LibraryManager.library, bf_LibWebGL);