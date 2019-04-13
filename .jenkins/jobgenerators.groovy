// Groovy methods for building Jenkins job stages for various platforms
// These generators are used for the platform specific testers

// Generator for Linux test
def buildLinuxJob(prefix, image, shared, extra_flags = '') {
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
            sh "cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=${shared} -DEMBDEBUG_ENABLE_WERROR=TRUE -DCMAKE_C_FLAGS='${extra_flags}' -DCMAKE_CXX_FLAGS='${extra_flags}' .."
            sh 'cmake --build . --target all'
          }
        }
      }
      stage("Linux ${prefix}: Test") {
        image.inside {
          dir('build') {
            sh 'CTEST_OUTPUT_ON_FAILURE=1 cmake --build . --target test'
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
          bat script: "cmake -G \"${generator}\" -A ${abi} -DCMAKE_BUILD_TYPE=Debug -DEMBDEBUG_ENABLE_WERROR=TRUE -Dgtest_force_shared_crt=TRUE .."
          bat script: 'cmake --build . --target ALL_BUILD'
        }
      }
      stage("MSVC ${version} ${abi}: Test") {
        dir('build') {
          bat script: 'set CTEST_OUTPUT_ON_FAILURE=1 && cmake --build . --target RUN_TESTS'
        }
      }
    }
  }
}

// Generator for macOS test
def buildMacOSJob(prefix, shared) {
  return {
    node('macos') {
      stage("macOS ${prefix}: Checkout") {
        deleteDir()
        checkout scm
      }
      stage("macOS ${prefix}: Build") {
        dir('build') {
          sh "cmake -DBUILD_SHARED_LIBS=${shared} -DCMAKE_BUILD_TYPE=Debug -DEMBDEBUG_ENABLE_WERROR=TRUE .."
          sh 'cmake --build . --target all'
        }
      }
      stage("macOS ${prefix}: Test") {
        dir('build') {
          sh 'CTEST_OUTPUT_ON_FAILURE=1 cmake --build . --target test'
        }
      }
    }
  }
}

// Generator for job that will fail if clang-format makes changes
def buildStyleJob() {
  return {
    node('linux-docker') {
      stage("Style: Checkout") {
        deleteDir()
        checkout scm
      }
      stage("Style: Build docker image") {
        image = docker.build("gdbserver-test-ubuntu1804-clang", "-f .jenkins/docker/linux-ubuntu1804-clang.dockerfile .jenkins/docker")
      }
      stage("Style: Configure") {
        image.inside {
          dir('build') {
            sh "cmake .."
          }
        }
      }
      stage("Style: Test") {
        image.inside {
          dir('build') {
            sh 'cmake --build . --target clang-format'
            sh 'git diff > style-differences.diff'
            def count = sh(script: 'cat style-differences.diff | wc -l', returnStdout: true) as Integer
            if (count > 0) {
              archiveArtifacts allowEmptyArchive: true, artifacts: 'style-differences.diff'
              currentBuild.result = 'UNSTABLE'
            }
          }
        }
      }
    }
  }
}

return this;
