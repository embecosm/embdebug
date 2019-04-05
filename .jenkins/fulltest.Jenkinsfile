// Jenkins pipeline for testing on all supported platforms

// Generator for Linux test
def buildLinuxJob(prefix, shared) {
  return {
    node('linux-docker') {
      stage("Linux ${prefix}: Checkout") {
        deleteDir()
        checkout scm
      }
      stage("Linux ${prefix}: Build docker image") {
        image = docker.build("gdbserver-test-${prefix}", "-f .jenkins/docker/linux-${image}.dockerfile .jenkins/docker")
      }
      stage("Linux ${prefix}: Build") {
        image.inside {
          dir('build') {
            sh "cmake -DBUILD_SHARED_LIBS=${shared} -DEMBDEBUG_ENABLE_WERROR=TRUE .."
            sh 'cmake --build . --target all'
          }
        }
      }
      stage("Linux ${prefix}: Test") {
        image.inside {
          dir('build') {
            sh 'cmake --build . --target test'
          }
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

// Linux targets
def LINUX_TARGETS = ['centos7-gcc7', 'centos7-gcc6', 'centos7-gcc5', 'centos7-clang',
                     'ubuntu1604-gcc', 'ubuntu1604-clang', 'ubuntu1804-gcc', 'ubuntu1804-clang']
LINUX_TARGETS.each { name -> 
  JOBS["linux-${name}-shared"] = buildLinuxJob("${name}-shared", name, true)
  JOBS["linux-${name}-static"] = buildLinuxJob("${name}-static", name, true)
}

// Windows targets
def VS_VERSIONS = ['2015': 'Visual Studio 14 2015',
                   '2017': 'Visual Studio 15 2017',
                   '2019': 'Visual Studio 16 2019']
VS_VERSIONS.each { vers, gen ->
  JOBS["windows-${vers}-Win32"] = buildWindowsJob(vers, gen, 'Win32')
  JOBS["windows-${vers}-x64"] = buildWindowsJob(vers, gen, 'x64')
}

// Run all jobs
parallel JOBS
