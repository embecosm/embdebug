// Jenkins pipeline for testing on all supported platforms

// Load generators
node('master') {
  checkout scm
  generators = load ".jenkins/jobgenerators.groovy"
}

// Map of all variants to run
def JOBS = [:]
// Map of variants to run
JOBS = [:]

// Linux targets
def LINUX_TARGETS = ['centos7-gcc7', 'centos7-gcc6', 'centos7-gcc5', 'centos7-clang',
                     'ubuntu1604-gcc', 'ubuntu1604-clang', 'ubuntu1804-gcc', 'ubuntu1804-clang']
LINUX_TARGETS.each { name -> 
  JOBS["linux-${name}-shared"] = generators.buildLinuxJob("${name}-shared", name, true)
  JOBS["linux-${name}-static"] = generators.buildLinuxJob("${name}-static", name, false)
}

// Windows targets
def VS_VERSIONS = ['2015': 'Visual Studio 14 2015',
                   '2017': 'Visual Studio 15 2017',
                   '2019': 'Visual Studio 16 2019']
VS_VERSIONS.each { vers, gen ->
  JOBS["windows-${vers}-Win32"] = generators.buildWindowsJob(vers, gen, 'Win32')
  JOBS["windows-${vers}-x64"] = generators.buildWindowsJob(vers, gen, 'x64')
}

// macOS targets
JOBS["macos-shared"] = generators.buildMacOSJob("Shared", true)
JOBS["macos-static"] = generators.buildMacOSJob("Static", false)

// Style test
JOBS["style"] = generators.buildStyleJob()

// Run jobs in parallel
parallel JOBS
