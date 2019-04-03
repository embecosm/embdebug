// Jenkins pipeline for testing on all supported platforms

// Generator for Linux test
def buildLinuxJob(prefix, shared) {
  return {
    node('linux') {
      stage("Linux ${prefix}: Checkout") {
        deleteDir()
        checkout scm
      }
      stage("Linux ${prefix}: Build") {
        dir('build') {
          sh "cmake -DBUILD_SHARED_LIBS=${shared} -DEMBDEBUG_ENABLE_WERROR=TRUE .."
          sh 'cmake --build . --target all'
        }
      }
      stage("Linux ${prefix}: Test") {
        dir('build') {
          sh 'cmake --build . --target test'
        }
      }
    }
  }
}

// Generator for Windows test
def buildWindowsJob(version, generator, abi) {
  return {
    node('windows') {
      stage("MSVC ${version} ${abi}: Checkout") {
        deleteDir()
        checkout scm
      }
      stage("MSVC ${version} ${abi}: Build") {
        dir('build') {
          bat script: "cmake -G \"${generator}\" -A ${abi}  -DEMBDEBUG_ENABLE_WERROR=TRUE -Dgtest_force_shared_crt=TRUE .."
          bat script: 'cmake --build . --target ALL_BUILD'
        }
      }
      stage("MSVC ${version} ${abi}: Test") {
        dir('build') {
          bat script: 'cmake --build . --target RUN_TESTS'
        }
      }
    }
  }
}

// Map of all variants to run
def JOBS = [:]
JOBS['linux-shared'] = buildLinuxJob('shared', true)
JOBS['linux-static'] = buildLinuxJob('static', false)

def VS_VERSIONS = ['2015': 'Visual Studio 14 2015',
                   '2017': 'Visual Studio 15 2017',
                   '2019': 'Visual Studio 16 2019']
VS_VERSIONS.each { vers, gen ->
  JOBS["windows-${vers}-Win32"] = buildWindowsJob(vers, gen, 'Win32')
  JOBS["windows-${vers}-x64"] = buildWindowsJob(vers, gen, 'x64')
}

// Run all jobs
parallel JOBS
