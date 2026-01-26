from imgui_bundle import imgui, ImVec2
from imgui_bundle.python_backends.glfw_backend import GlfwRenderer
import glfw
import numpy as np
from OpenGL import GL


class GUI_Context:
    def __init__(self):
        self._init_window()
        self._init_imgui()
        self._apply_styling()

    def _init_window(self):
        if not glfw.init():
            raise Exception("GLFW can't be initialized")
        # Important: request an OpenGL 3.3 core profile context on EGL
        glfw.window_hint(glfw.CONTEXT_CREATION_API, glfw.EGL_CONTEXT_API)
        glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
        glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
        glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
        glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, GL.GL_TRUE)
        self.window = glfw.create_window(800, 600, "OpenGL Window", None, None)
        if not self.window:
            glfw.terminate()
            raise Exception("GLFW window can't be created")

    def _init_imgui(self):
        glfw.make_context_current(self.window)
        imgui.create_context()
        self.imgui_impl = GlfwRenderer(self.window)
        
    def _apply_styling(self):
        pass # TODO

    def delete(self) -> None:
        self.imgui_impl.shutdown()
        glfw.destroy_window(self.window)
        glfw.terminate()

    def should_close(self) -> bool:
        return glfw.window_should_close(self.window)


class GL_Shader:
    def __init__(self, shader_sources: dict[any, str]):
        self.id = GL.glCreateProgram()
        shaders = [self._compile_shader(source, shader_type) 
                   for shader_type, source in shader_sources.items()]
        for shader in shaders:
            GL.glAttachShader(self.id, shader)
        GL.glLinkProgram(self.id)
        for shader in shaders:
            GL.glDeleteShader(shader)

    def _compile_shader(self, source: str, shader_type: int) -> int:
        shader = GL.glCreateShader(shader_type)
        GL.glShaderSource(shader, source)
        GL.glCompileShader(shader)
        if not GL.glGetShaderiv(shader, GL.GL_COMPILE_STATUS):
            error = GL.glGetShaderInfoLog(shader).decode()
            raise RuntimeError(f"Shader compilation error: {error}")
        return shader

    def use(self) -> None:
        GL.glUseProgram(self.id)


class GL_Texture:
    def __init__(self, image_data: np.ndarray):
        self.id = GL.glGenTextures(1)
        GL.glBindTexture(GL.GL_TEXTURE_2D, self.id)
        GL.glTexImage2D(GL.GL_TEXTURE_2D, 0, GL.GL_RGBA, image_data.shape[1], image_data.shape[0],
                        0, GL.GL_RGBA, GL.GL_UNSIGNED_BYTE, image_data)
        GL.glGenerateMipmap(GL.GL_TEXTURE_2D)

    def bind(self) -> None:
        GL.glBindTexture(GL.GL_TEXTURE_2D, self.id)


class GL_Mesh:
    def __init__(self, vertices: np.ndarray, indices: np.ndarray):
        self.vao = GL.glGenVertexArrays(1)
        self.vbo = GL.glGenBuffers(1)
        self.ebo = GL.glGenBuffers(1)

        GL.glBindVertexArray(self.vao)

        GL.glBindBuffer(GL.GL_ARRAY_BUFFER, self.vbo)
        GL.glBufferData(GL.GL_ARRAY_BUFFER, vertices.nbytes, vertices, GL.GL_STATIC_DRAW)

        GL.glBindBuffer(GL.GL_ELEMENT_ARRAY_BUFFER, self.ebo)
        GL.glBufferData(GL.GL_ELEMENT_ARRAY_BUFFER, indices.nbytes, indices, GL.GL_STATIC_DRAW)

        # Assuming vertices contain position (3 floats) and color (3 floats)
        GL.glVertexAttribPointer(0, 3, GL.GL_FLOAT, GL.GL_FALSE, 6 * vertices.itemsize, GL.ctypes.c_void_p(0))
        GL.glEnableVertexAttribArray(0)
        GL.glVertexAttribPointer(1, 3, GL.GL_FLOAT, GL.GL_FALSE, 6 * vertices.itemsize, GL.ctypes.c_void_p(3 * vertices.itemsize))
        GL.glEnableVertexAttribArray(1)

        GL.glBindBuffer(GL.GL_ARRAY_BUFFER, 0)
        GL.glBindVertexArray(0)

    def draw(self) -> None:
        GL.glBindVertexArray(self.vao)
        GL.glDrawElements(GL.GL_TRIANGLES, 36, GL.GL_UNSIGNED_INT, None)
        GL.glBindVertexArray(0)


scene = None


def imgui_main() -> None:
    imgui.set_next_window_pos(ImVec2(0, 0))
    imgui.set_next_window_size(ImVec2(1280, 720))
    imgui.begin("MainWindow", flags=imgui.WindowFlags_.no_title_bar 
                            | imgui.WindowFlags_.no_resize
                            | imgui.WindowFlags_.no_move
                            | imgui.WindowFlags_.no_scrollbar
                            | imgui.WindowFlags_.no_inputs)
    imgui.text("Hello world, this is on the main window!")
    imgui.end()


def render_main() -> None:
    global scene
    scene['shader'].use()
    scene['mesh'].draw()


def gui_loop(context: GUI_Context) -> None:
    while not context.should_close():
        context.imgui_impl.process_inputs()
        imgui.new_frame()
        imgui_main()
        imgui.render()
        GL.glClearColor(0.1, 0.1, 0.1, 1.0)
        GL.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT)
        context.imgui_impl.render(imgui.get_draw_data())
        render_main()
        glfw.swap_buffers(context.window)
        glfw.poll_events()


def main() -> None:
    gui_context = GUI_Context()

    global scene
    scene = {}
    scene['shader'] = GL_Shader({
        GL.GL_VERTEX_SHADER: """
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        out vec3 ourColor;
        void main()
        {
            gl_Position = vec4(aPos, 1.0);
            ourColor = aColor;
        }
        """,
        GL.GL_FRAGMENT_SHADER: """
        #version 330 core
        out vec4 FragColor;
        in vec3 ourColor;
        void main()
        {
            FragColor = vec4(ourColor, 1.0);
        }
        """
    })

    scene['mesh'] = GL_Mesh(
        np.array([-1.0, -1.0, 0.0, 1.0, 0.0, 0.0,
                   1.0, -1.0, 0.0, 0.0, 1.0, 0.0,
                   0.0,  1.0, 0.0, 0.0, 0.0, 1.0], dtype=np.float32),
        np.array([0, 1, 2], dtype=np.uint32)
    )

    gui_loop(gui_context)
    gui_context.delete()


if __name__ == "__main__":
    main()
