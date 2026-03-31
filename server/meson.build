# It may be useful to use a prebuilt server, so that no Android SDK is required
# to build. If the 'prebuilt_server' option is set, just copy the file as is.
prebuilt_server = get_option('prebuilt_server')
if prebuilt_server == ''
    custom_target('scrcpy-server',
                  # gradle is responsible for tracking source changes
                  build_by_default: true,
                  build_always_stale: true,
                  output: 'scrcpy-server',
                  command: [find_program('./scripts/build-wrapper.sh'), meson.current_source_dir(), '@OUTPUT@', get_option('buildtype')],
                  console: true,
                  install: true,
                  install_dir: 'share/scrcpy')
else
    # get_option() yields OptionStringHolder, not str — coerce before path checks.
    ps = '' + prebuilt_server
    # Relative paths are from the repo root. Unix absolute paths start with '/'; Windows
    # paths are normalized to 'C:/...' (second char is ':').
    is_abs = ps.startswith('/') or ps.contains(':/')
    if not is_abs
        ps = '../' + ps
    endif
    custom_target('scrcpy-server-prebuilt',
                  input: ps,
                  output: 'scrcpy-server',
                  command: ['cp', '@INPUT@', '@OUTPUT@'],
                  install: true,
                  install_dir: 'share/scrcpy')
endif

if meson.version().version_compare('>= 0.58.0')
       devenv = environment()
       devenv.set('SCRCPY_SERVER_PATH', meson.current_build_dir() / 'scrcpy-server')
       meson.add_devenv(devenv)
endif
