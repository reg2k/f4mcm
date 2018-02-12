import os

# Configuration
BUILD_DIR        = 'build'
PROJECT_DIR      = 'src'
PLATFORM_TOOLSET = 'v140'
F4SE_REVISION    = 'master'
BUILD_TOOLS_REPO = 'https://github.com/reg2k/f4se-build-tools.git'

# Fetch F4SE build tools
if not os.path.exists('{}/build-tools'.format(BUILD_DIR)):
    os.system('git clone {} {}/build-tools'.format(BUILD_TOOLS_REPO, BUILD_DIR))

# Run build tools
os.system('python {}/build-tools/build_plugin.py "{}" "{}" "{}" "{}"'
    .format(BUILD_DIR, BUILD_DIR, PROJECT_DIR, PLATFORM_TOOLSET, F4SE_REVISION))