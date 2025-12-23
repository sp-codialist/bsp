require "#{ENV['CMOCK_DIR']}/lib/cmock"

raise 'Header file to mock must be specified!' unless ARGV.length >= 1

mock_out = ENV.fetch('MOCK_OUT', './build/test/mocks')
mock_prefix = ENV.fetch('MOCK_PREFIX', 'mock_')
cmock = CMock.new(
  :plugins => %i[ignore ignore_arg callback],
  :mock_prefix => mock_prefix,
  :mock_path => mock_out,
  :includes => %i[stdint.h math.h stdbool.h],
  :strippables => ['CCM_DECL_INSTANCE_HDL_', 'CCM_DECL_INSTANCE_HDL', 'CCM_DECL_PTR_INSTANCE_HDL_', 'CCM_DECL_PTR_INSTANCE_HDL', 'COPDLLEXPORT', 'PUBLIC(?=\s)'],
  :treat_as_void => ['CCM_DECL_INSTANCE_HDL'],
  :treat_as => {
    'BYTE' => 'UINT8',
    'WORD' => 'UINT16',
    'DWORD' => 'UINT32'
  }
)
cmock.setup_mocks(ARGV[0])
