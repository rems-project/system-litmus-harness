BRANCH="$(git branch --show-current)"
make html
git add -f build/html/
git commit -m "tmp"
git stash
pushd ..
git checkout gh-pages
git restore --source=${BRANCH} doc/build/html
cp -r doc/build/html/* .
find doc/build/html | while read fname; do
    [[ -f $fname ]] || continue;
    git add -f "$(echo $fname | cut -c 16-)"
done
git status
git commit -m "make publish -- $(date)"
git checkout ${BRANCH}
git stash pop
git reset HEAD~
popd