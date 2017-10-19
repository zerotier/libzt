#!/usr/bin/env groovy

node('master') {
    checkout scm
    def changelog = getChangeLog currentBuild
    mattermostSend "Building ${env.JOB_NAME} #${env.BUILD_NUMBER} \n Change Log: \n ${changelog}"
}

parallel 'centos7': {
	node('centos7') {

	// ------------------------------------------------------------------------------
	// ---------------------------- static library (Linux) --------------------------
	// ------------------------------------------------------------------------------

// NO_STACK

		try {
			checkout scm
			sh 'git submodule update --init'
			stage('linux static lib, no stack') {
				sh 'make clean; make -j4 static_lib NO_STACK=1; make tests'
			}
		}
		catch (err) {
			currentBuild.result = "FAILURE"
			slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
			throw err
		}

/* 
// picoTCP

		try {
			checkout scm
			sh 'git submodule update --init'
			stage('linux static lib, picoTCP') {
				sh 'make clean; make -j4 static_lib STACK_PICO=1; make tests'
			}
		}
		catch (err) {
			currentBuild.result = "FAILURE"
			slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
			throw err
		}


		try {
			checkout scm
			sh 'git submodule update --init'
			stage('linux static lib, picoTCP, ipv4') {
				sh 'make clean; make -j4 static_lib STACK_PICO=1 LIBZT_IPV4=1; make tests'
			}
		}
		catch (err) {
			currentBuild.result = "FAILURE"
			slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
			throw err
		}


		try {
			checkout scm
			sh 'git submodule update --init'
			stage('linux static lib, picoTCP, ipv6') {
				sh 'make clean; make -j4 static_lib STACK_PICO=1 LIBZT_IPV6=1; make tests'
			}
		}
		catch (err) {
			currentBuild.result = "FAILURE"
			slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
			throw err
		}


		try {
			checkout scm
			sh 'git submodule update --init'
			stage('linux static lib, picoTCP, ipv4, ipv6') {
				sh 'make clean; make -j4 static_lib STACK_PICO=1 LIBZT_IPV4=1 LIBZT_IPV6=1; make tests'
			}
		}
		catch (err) {
			currentBuild.result = "FAILURE"
			slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
			throw err
		}
*/

// lwIP


		try {
			checkout scm
			sh 'git submodule update --init'
			stage('linux static lib lwIP') {
				sh 'make clean; make -j4 static_lib STACK_LWIP=1; make tests'
			}
		}
		catch (err) {
			currentBuild.result = "FAILURE"
			slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
			throw err
		}

	}
}, 'macOS': {
	node('macOS') {

		unlockKeychainMac "~/Library/Keychains/login.keychain-db"

	// ------------------------------------------------------------------------------
	// -------------------------- Intercept Library (macOS) -------------------------
	// ------------------------------------------------------------------------------

	// TODO

	// ------------------------------------------------------------------------------
	// ---------------------------- App Frameworks (macOS) --------------------------
	// ------------------------------------------------------------------------------

	// TODO

	// ------------------------------------------------------------------------------
	// ----------------------------- static library (macOS) -------------------------
	// ------------------------------------------------------------------------------

// NO_STACK

		try {
			checkout scm
			sh 'git submodule update --init'
			stage('macOS static lib, no stack') {
				sh 'make clean; make -j4 static_lib NO_STACK=1; make tests'
			}
		}
		catch (err) {
			currentBuild.result = "FAILURE"
			slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
			throw err
		}

/* 

// picoTCP

		try {
			checkout scm
			sh 'git submodule update --init'
			stage('macOS static lib, picoTCP') {
				sh 'make clean; make -j4 static_lib STACK_PICO=1; make tests'
			}
		}
		catch (err) {
			currentBuild.result = "FAILURE"
			slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
			throw err
		}


		try {
			checkout scm
			sh 'git submodule update --init'
			stage('macOS static lib, picoTCP, ipv4') {
				sh 'make clean; make -j4 static_lib STACK_PICO=1 LIBZT_IPV4=1; make tests'
			}
		}
		catch (err) {
			currentBuild.result = "FAILURE"
			slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
			throw err
		}


		try {
			checkout scm
			sh 'git submodule update --init'
			stage('macOS static lib, picoTCP, ipv6') {
				sh 'make clean; make -j4 static_lib STACK_PICO=1 LIBZT_IPV6=1; make tests'
			}
		}
		catch (err) {
			currentBuild.result = "FAILURE"
			slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
			throw err
		}


		try {
			checkout scm
			sh 'git submodule update --init'
			stage('macOS static lib, picoTCP, ipv4, ipv6') {
				sh 'make clean; make -j4 static_lib STACK_PICO=1 LIBZT_IPV4=1 LIBZT_IPV6=1; make tests'
			}
		}
		catch (err) {
			currentBuild.result = "FAILURE"
			slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
			throw err
		}

*/

// lwIP


		try {
			checkout scm
			sh 'git submodule update --init'
			stage('macOS static lib, lwIP') {
				sh 'make clean; make -j4 static_lib STACK_LWIP=1; make tests'
			}
		}
		catch (err) {
			currentBuild.result = "FAILURE"
			slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
			throw err
		}
	}
}


mattermostSend color: "#00ff00", message: "${env.JOB_NAME} #${env.BUILD_NUMBER} Complete (<${env.BUILD_URL}|Show More...>)"