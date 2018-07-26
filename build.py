import os, sys

# Configuration
BUILD_DIR        = 'build'
PROJECT_DIR      = 'src'
PLATFORM_TOOLSET = 'v140'
F4SE_REVISION    = 'tags/v0.6.11'

# Run build tools
buildOK = os.system('python tools/build-tools/build_plugin.py "{}" "{}" "{}" "{}"'
    .format(BUILD_DIR, PROJECT_DIR, PLATFORM_TOOLSET, F4SE_REVISION))
    
# Report result
sys.exit(buildOK)
