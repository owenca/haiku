/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/AddRepositoryRequest.h>

#include <Directory.h>
#include <Path.h>

#include <package/ActivateRepositoryConfigJob.h>
#include <package/FetchFileJob.h>
#include <package/JobQueue.h>
#include <package/PackageRoster.h>


namespace BPackageKit {


using namespace BPrivate;


AddRepositoryRequest::AddRepositoryRequest(const BContext& context,
	const BString& repositoryBaseURL, bool asUserRepository)
	:
	inherited(context),
	fRepositoryBaseURL(repositoryBaseURL),
	fAsUserRepository(asUserRepository),
	fActivateJob(NULL)
{
}


AddRepositoryRequest::~AddRepositoryRequest()
{
}


status_t
AddRepositoryRequest::CreateInitialJobs()
{
	status_t result = InitCheck();
	if (result != B_OK)
		return B_NO_INIT;

	BEntry tempEntry;
	result = fContext.GetNewTempfile("repoheader-", &tempEntry);
	if (result != B_OK)
		return result;
	BString repoHeaderURL = BString(fRepositoryBaseURL) << "/" << "repo.header";
	FetchFileJob* fetchJob = new (std::nothrow) FetchFileJob(fContext,
		BString("Fetching repository header from ") << fRepositoryBaseURL,
		repoHeaderURL, tempEntry);
	if (fetchJob == NULL)
		return B_NO_MEMORY;
	if ((result = QueueJob(fetchJob)) != B_OK) {
		delete fetchJob;
		return result;
	}

	BPackageRoster roster;
	BPath targetRepoConfigPath;
	result = fAsUserRepository
		? roster.GetUserRepositoryConfigPath(&targetRepoConfigPath, true)
		: roster.GetCommonRepositoryConfigPath(&targetRepoConfigPath, true);
	if (result != B_OK)
		return result;
	BDirectory targetDirectory(targetRepoConfigPath.Path());
	ActivateRepositoryConfigJob* activateJob
		= new (std::nothrow) ActivateRepositoryConfigJob(fContext,
			BString("Activating repository config from ") << fRepositoryBaseURL,
			tempEntry, fRepositoryBaseURL, targetDirectory);
	if (activateJob == NULL)
		return B_NO_MEMORY;
	activateJob->AddDependency(fetchJob);
	if ((result = QueueJob(activateJob)) != B_OK) {
		delete activateJob;
		return result;
	}
	fActivateJob = activateJob;

	return B_OK;
}


void
AddRepositoryRequest::JobSucceeded(BJob* job)
{
	if (job == fActivateJob)
		fRepositoryName = fActivateJob->RepositoryName();
}


const BString&
AddRepositoryRequest::RepositoryName() const
{
	return fRepositoryName;
}


}	// namespace BPackageKit