pipeline {
  agent { label 'macos' }
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
        // This builds and tests an explicitly static build
        stage('Static') {
          stages {
            stage('Build') {
              steps {
                dir('build-static') {
                  sh 'cmake -DBUILD_SHARED_LIBS=TRUE -DEMBDEBUG_ENABLE_WERROR=TRUE ..'
                  sh 'cmake --build . --target all'
                }
              }
            }
            stage('Test') {
              steps {
                dir('build-static') {
                  sh 'cmake --build . --target test'
                }
              }
            }
          }
        }

        // This builds and tests an explicitly shared build
        stage('Shared') {
          stages {
            stage('Build') {
              steps {
                dir('build-shared') {
                  sh 'cmake -DBUILD_SHARED_LIBS=TRUE -DEMBDEBUG_ENABLE_WERROR=TRUE ..'
                  sh 'cmake --build . --target all'
                }
              }
            }
            stage('Test') {
              steps {
                dir('build-shared') {
                  sh 'cmake --build . --target test'
                }
              }
            }
          }
        }
      }
    }
  }
}
