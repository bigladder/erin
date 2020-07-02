#!/user/bin/env ruby
# Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
# See the LICENSE file for additional terms and conditions.
######################################################################
# This is a unit test file for template.toml and various examples of exercising it.
# This module uses Ruby's standard unit test library, MiniTest
# This library assumes that Modelkit is installed and available from the
# commandline
require 'minitest/autorun'
require 'open3'
require 'stringio'

THIS_DIR = File.expand_path(File.dirname(__FILE__))

class TestTemplate < Minitest::Test
  def call_modelkit(args, output_file, dir)
    cmd = "modelkit template-compose #{args}"
    val = {}
    Dir.chdir(dir) do
      out, err, s = Open3.capture3(cmd, chdir: THIS_DIR)
      exist = File.exist?(output_file)
      val[:cmd] = cmd
      val[:success] = s.success? && exist
      val[:exit] = s.success?
      val[:output_exists] = exist
      val[:output] = nil
      val[:stdout] = out
      val[:stderr] = err
      val[:output] = File.read(output_file) if exist
    end
    return val
  end

  def error_string(v)
    s = StringIO.new
    s.write("ERROR:\n")
    v.keys.sort.each do |key|
      s.write("#{key.to_s}: #{v[key]}\n")
    end
    s.string
  end

  def setup
    @input_file = "ex.pxt"
    @output_file = "test.toml"
  end

  def teardown
    File.delete(@output_file) if File.exist?(@output_file)
  end

  def test_ex_template_expected
    template_file = "ex.pxt"
    out_file = File.join(THIS_DIR, @output_file)
    tmp_file = File.join(THIS_DIR, template_file)
    out = call_modelkit("--dirs=\"#{THIS_DIR}\" --output=\"#{out_file}\" \"#{tmp_file}\"", out_file, THIS_DIR)
    err_str = error_string(out)
    assert(out[:success], err_str)
    assert(!out[:output].nil?, err_str)
    expected = File.read(File.join(THIS_DIR, 'reference', 'expected_ex.toml'))
    assert_equal(out[:output], expected, err_str)
  end
end
