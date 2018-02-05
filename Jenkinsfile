#!/usr/bin/env groovy

node('master') {
    checkout scm
    def changelog = getChangeLog currentBuild
    mattermostSend "Building ${env.JOB_NAME} #${env.BUILD_NUMBER} \n Change Log: \n ${changelog}"
}

parallel 'centos7': {
	node('centos7') {
		try {
			checkout scm
			sh 'git submodule update --init'
			stage('linux') {
				sh 'cmake -H. -Bbuild; cmake --build build' 
			}
		}
		catch (err) {
			currentBuild.result = "FAILURE"
			slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
			throw err
		}
	}
}, 

'macOS': {
	node('macOS') {
		unlockKeychainMac "~/Library/Keychains/login.keychain-db"
		try {
			checkout scm
			sh 'git submodule update --init'
			stage('macOS') {
				sh 'cmake -H. -Bbuild; cmake --build build' 
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