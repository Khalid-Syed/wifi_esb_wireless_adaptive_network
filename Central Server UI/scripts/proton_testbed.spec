# -*- mode: python ; coding: utf-8 -*-

"""PyInstaller spec for building the PROTON Testbed GUI.

Author: Md Sadman Siraj
Email: msiraj13@asu.edu
Date: 2026-02-02
"""

import os

project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
src_path = os.path.join(project_root, "src")
icon_path = os.path.join(project_root, "assets", "image.ico")
entry_script = os.path.join(project_root, "proton_testbed.py")


a = Analysis(
    [entry_script],
    pathex=[src_path],
    binaries=[],
    datas=[(icon_path, "assets")],
    hiddenimports=[],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name="proton_testbed",
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon=[icon_path],
)
