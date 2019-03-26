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

    stage('Build') {
      steps {
        dir('build') {
          bat script: 'cmake -DEMBDEBUG_ENABLE_WERROR=TRUE -Dgtest_force_shared_crt=TRUE ..'
          bat script: 'cmake --build . --target ALL_BUILD'
        }
      }
    }

    stage('Test') {
      steps {
        dir('build') {
          bat script: 'cmake --build . --target RUN_TESTS'
        }
      }
    }
  }
}
