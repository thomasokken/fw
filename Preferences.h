class Preferences {
    public:
	int verbosity;
	bool x_errors_coredump;
	bool no_priv_cmap_in_cpicker;

	Preferences(int *argv, char *argv[]);
	~Preferences();
};
