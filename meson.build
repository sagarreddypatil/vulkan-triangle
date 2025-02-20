
project('LearnVulkan', 'cpp',
  version : '1.0.0',
  default_options : ['cpp_std=c++17'],
  meson_version: '>=1.4.0'
)

# Find required dependencies
vulkan_dep = dependency('vulkan')
glfw_dep = dependency('glfw3')
glm_dep = dependency('glm')

# Define source files
src_files = [
  'src/main.cpp',
  # Add other source files here
]

# Create executable
learn_vulkan = executable('LearnVulkan',
  sources : src_files,
  dependencies : [
    vulkan_dep,
    glfw_dep,
    glm_dep
  ]
)

# Shader compilation
glslang = find_program('glslangValidator')

shader_files = files(
  'src/shaders/tri.vert',
  'src/shaders/tri.frag'
)

# Create shader compilation target
foreach shader : shader_files
  fname = '@0@'.format(shader.full_path()).split('/')[-1]
  custom_target(
    fname,
    input : shader,
    output : fname + '.spv',
    command : [glslang, '-V', '@INPUT@', '-o', '@OUTPUT@'],
    build_by_default : true,
  )
endforeach