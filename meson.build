project('blobomnom', 'c',
        version: '0.1',
        meson_version: '>= 0.49.2',
        default_options: ['buildtype=release', 'optimization=s', 'unity=on', 'b_lto=true', 'b_pie=true', 'c_link_args=-static', 'c_std=c99'],
        license: 'UNLICENSE')

install_man('blobomnom.1')

blobomnom_exe = executable('blobomnom',
  ['main.c'],
  install: true
)
