# Copyright (C) 2012 The Android Open Source Project
# Copyright (C) 2012 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Custom OTA commands for wave"""

import common
import os

TARGET_DIR = os.getenv('OUT')
UTILITIES_DIR = os.path.join(TARGET_DIR, 'utilities')

def FullOTA_Assertions(info):

  info.output_zip.write(os.path.join(UTILITIES_DIR, "make_ext4fs"), "make_ext4fs")
  info.output_zip.write(os.path.join(UTILITIES_DIR, "busybox"), "busybox")
  info.output_zip.write(os.path.join(UTILITIES_DIR, "flash_image"), "flash_image")

  info.script.AppendExtra(
        ('package_extract_file("system/bin/updater.sh", "/tmp/updater.sh");\n'
         'set_perm(0, 0, 0777, "/tmp/updater.sh");'))
  info.script.AppendExtra(
       ('package_extract_file("make_ext4fs", "/tmp/make_ext4fs");\n'
        'set_perm(0, 0, 0777, "/tmp/make_ext4fs");'))
  info.script.AppendExtra(
        ('package_extract_file("busybox", "/tmp/busybox");\n'
         'set_perm(0, 0, 0777, "/tmp/busybox");'))
  info.script.AppendExtra(
        ('package_extract_file("flash_image", "/tmp/flash_image");\n'
         'set_perm(0, 0, 0777, "/tmp/flash_image");'))

  info.script.AppendExtra('package_extract_file("boot.img", "/tmp/boot.img");')
  info.script.AppendExtra('assert(run_program("/tmp/updater.sh") == 0);')


def FullOTA_InstallEnd(info):
  # Remove writing boot.img from script (we do it in updater.sh)
  info.script.script = [cmd for cmd in info.script.script if not "write_raw_image" in cmd]

  # Run model specify script
  info.script.AppendExtra(
        ('package_extract_file("system/bin/setmodel.sh", "/tmp/setmodel.sh");\n'
         'set_perm(0, 0, 0777, "/tmp/setmodel.sh");\n'
         'assert(run_program("/tmp/setmodel.sh") == 0);'))
