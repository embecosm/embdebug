// Windows smoke test

// Load generators
node('master') {
  checkout scm
  generators = load '.jenkins/jobgenerators.groovy'
}

// Run Windows tests for VS2017
JOBS = [:]
JOBS['Win32'] = generators.buildWindowsJob('2017', 'Visual Studio 15 2017', 'Win32')
JOBS['x64'] = generators.buildWindowsJob('2017', 'Visual Studio 15 2017', 'x64')
parallel JOBS
