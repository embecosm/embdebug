pipeline {
  agent { label 'windows' }
  options { skipDefaultCheckout() }

  stages {
    stage('Checkout') {
      steps {
        deleteDir()
        checkout scm
      }
    }

    stage('Build and test') {
      parallel {
        // This builds for the Win32 platform
        stage('Win32') {
          stages {
            stage('Build') {
              steps {
                dir('build-win32') {
                  bat script: 'cmake -A Win32 -DEMBDEBUG_ENABLE_WERROR=TRUE -Dgtest_force_shared_crt=TRUE ..'
                  bat script: 'cmake --build . --target ALL_BUILD'
                }
              }
            }

            stage('Test') {
              steps {
                dir('build-win32') {
                  bat script: 'cmake --build . --target RUN_TESTS'
                }
              }
            }
          }
        }

        // This builds for the x64 platform
        stage('x64') {
          stages {
            stage('Build') {
              steps {
                dir('build-x64') {
                  bat script: 'cmake -A x64 -DEMBDEBUG_ENABLE_WERROR=TRUE -Dgtest_force_shared_crt=TRUE ..'
                  bat script: 'cmake --build . --target ALL_BUILD'
                }
              }
            }

            stage('Test') {
              steps {
                dir('build-x64') {
                  bat script: 'cmake --build . --target RUN_TESTS'
                }
              }
            }
          }
        }
      }
    }
  }
}
