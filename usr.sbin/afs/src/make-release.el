; $Id: make-release.el,v 1.3 2000/08/19 19:22:15 assar Exp $

(defun bump-version (filename prefix v-string)
  (save-excursion
    (find-file filename)
    (while (re-search-forward (concat prefix "[0-9\\.]*[0-9]") nil t)
      (replace-match v-string nil nil))
    (save-buffer)))

(let* ((version (getenv "HV"))
       (arla-version (concat "arla-" version))
       (version-string (concat "Release " version)))
  (find-file "configure.in")
  (re-search-forward "VERSION=\\(.*\\)$")
  (replace-match version nil nil nil 1)
  (save-buffer)
  (find-file "ChangeLog")
  (add-change-log-entry nil nil nil nil)
  (insert version-string)
  (save-buffer)
  (mapcar (function (lambda (v) (bump-version v "arla-" arla-version)))
	  '("README" "LIESMICH")) 
  (mapcar (function (lambda (v) (bump-version v "version " version-string)))
	  '("doc/arla.texi")) 
  (kill-emacs))
