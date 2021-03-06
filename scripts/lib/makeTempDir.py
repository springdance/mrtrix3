def makeTempDir(verbose):
  import os, random, string, sys
  from readMRtrixConfSetting import readMRtrixConfSetting
  dir_path = readMRtrixConfSetting('TmpFileDir')
  if not dir_path:
    dir_path = '.'
  prefix = readMRtrixConfSetting('TmpFilePrefix')
  if not prefix:
    prefix = os.path.basename(sys.argv[0]) + '-tmp-'
  full_path = dir_path
  while os.path.isdir(full_path):
    random_string = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(6))
    full_path = os.path.join(dir_path, prefix + random_string) + os.sep
  os.makedirs(full_path)
  if verbose:
    sys.stdout.write(os.path.basename(sys.argv[0]) + ': Generated temporary directory: ' + full_path + '\n')
    sys.stdout.flush()
  return full_path

