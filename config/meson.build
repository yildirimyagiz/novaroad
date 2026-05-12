project('nova', 'c', 'cpp',
  version: '0.1.0',
  default_options: [
    'c_std=c11',
    'cpp_std=c++17',
    'warning_level=3',
    'werror=false',
  ],
  license: 'MIT',
  meson_version: '>=0.60.0'
)

# Build options
nova_build_kernel = get_option('build_kernel')
nova_build_runtime = get_option('build_runtime')
nova_build_compiler = get_option('build_compiler')
nova_build_ai = get_option('build_ai')
nova_build_tools = get_option('build_tools')
nova_build_tests = get_option('build_tests')

# Platform detection
host_arch = host_machine.cpu_family()
if host_arch == 'x86_64'
  nova_arch = 'x86_64'
elif host_arch == 'aarch64'
  nova_arch = 'aarch64'
elif host_arch == 'riscv64'
  nova_arch = 'riscv64'
else
  nova_arch = 'generic'
endif

message('Building for architecture: ' + nova_arch)

# Configuration data
conf_data = configuration_data()
conf_data.set('NOVA_VERSION_MAJOR', 0)
conf_data.set('NOVA_VERSION_MINOR', 1)
conf_data.set('NOVA_VERSION_PATCH', 0)
conf_data.set_quoted('NOVA_ARCH', nova_arch)

configure_file(
  input: 'include/config.h.in',
  output: 'config.h',
  configuration: conf_data,
  install_dir: get_option('includedir') / 'nova'
)

# Include directories
inc_dirs = include_directories('include', 'build/include')

# Subdirectories
subdir('third_party')

if nova_build_kernel
  subdir('src/kernel')
endif

if nova_build_runtime
  subdir('src/runtime')
endif

if nova_build_compiler
  subdir('src/compiler')
endif

if nova_build_ai
  subdir('src/ai')
endif

subdir('src/security')
subdir('src/std')

if nova_build_tools
  subdir('tools')
endif

if nova_build_tests
  subdir('tests')
endif
