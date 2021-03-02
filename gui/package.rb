# Call this script to package up the Excel UI and required scripts
# Prior to calling this file, be sure to:
# 1. build the executable (erin_multi.exe)
# 2. build the User Guide (docs/user-guide)
# 3. update and test the support and template libs (docs/template/)
require 'fileutils'


PACKAGE_DIR = File.join(__dir__, 'packaged-gui')
TEMPLATE_DIR = File.join(__dir__, '..', 'docs', 'templates')
EXCEL_GUI = File.join(__dir__, 'erin_gui.xlsm')
SUPPORT_FILE = File.join(TEMPLATE_DIR, 'support.rb')
TEMPLATE_FILE = File.join(TEMPLATE_DIR, 'template.toml')
USER_GUIDE_FILE = File.join(
  __dir__, '..', 'docs', 'user-guide', 'user-guide.md.pdf')
EXE_DIR = File.join(
  __dir__, '..', 'out', 'build', 'x64-Release', 'bin')
ERIN_MULTI = File.join(EXE_DIR, 'erin_multi.exe')
ERIN = File.join(EXE_DIR, 'erin.exe')
ERIN_GRAPH = File.join(EXE_DIR, 'erin_graph.exe')
ERIN_DIST_TEST = File.join(EXE_DIR, 'erin_distribution_test.exe')
EXAMPLE_LOAD_CSV = File.join(
  __dir__, '..', 'docs', 'examples', 'ex02.csv')
LICENSE = File.join(__dir__, '..', 'LICENSE.txt')


def to_package_as(file_name)
  File.join(PACKAGE_DIR, file_name)
end


def to_package(path)
  to_package_as(File.basename(path))
end


MANIFEST = {
  EXCEL_GUI => to_package(EXCEL_GUI),
  SUPPORT_FILE => to_package(SUPPORT_FILE),
  TEMPLATE_FILE => to_package(TEMPLATE_FILE),
  USER_GUIDE_FILE => to_package_as('user-guide.pdf'),
  ERIN => to_package(ERIN),
  ERIN_MULTI => to_package(ERIN_MULTI),
  ERIN_GRAPH => to_package(ERIN_GRAPH),
  ERIN_DIST_TEST => to_package(ERIN_DIST_TEST),
  LICENSE => to_package(LICENSE),
  EXAMPLE_LOAD_CSV => to_package_as('example-load.csv'),
}


def make_dir_if_not_exist(dir)
  FileUtils.mkdir_p(dir) unless File.exist?(dir)
end


def check_manifest_existence
  MANIFEST.keys.each do |path|
    if !File.exists?(path)
      puts "Source file \"#{path}\" does not exist..."
      exit(1)
    end
  end
end


def display_manifest_modification_times
  MANIFEST.keys.each do |path|
    puts "#{path}:\n... #{File.mtime(path)}"
  end
end


def copy_manifest
  MANIFEST.each do |src, tgt|
    FileUtils.cp(src, tgt)
  end
end


def package
  puts "Packaging..."
  make_dir_if_not_exist(PACKAGE_DIR)
  check_manifest_existence
  display_manifest_modification_times
  copy_manifest
  puts "Packaging finished!"
end


package
