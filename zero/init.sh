#!/ bin / bash

#Script to clone the required repositories

#List of repositories to clone
repos = ("git://git.yoctoproject.org/meta-raspberrypi"
	 "git://git.yoctoproject.org/poky"
	 "git://git.openembedded.org/meta-openembedded"
	 "https://github.com/rauc/meta-rauc.git"
	 "https://github.com/rauc/meta-rauc-community.git")

	branch = "kirkstone"

#Function to clone a repository if it doesn't already exist
	clone_repo()
{
	repo_url = $1 repo_name = $(basename "$repo_url".git)

		if[!-d "$repo_name"];
	then echo "Cloning $repo_name..." git clone - b "$branch"
							"$repo_url" else echo
		"$repo_name already exists, skipping." fi
}

#Iterate over and clone each repository
for{
	repo in "${repos[@]}";
}
do
clone_repo "$repo" done

	echo "All repositories have been cloned."
