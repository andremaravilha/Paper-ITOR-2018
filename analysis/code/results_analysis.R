# ==============================================================================
# Clear workspace

rm(list = ls())


# ==============================================================================
# Install dependencies

dependencies.list <- c(
  "dplyr",
  "ggplot2",
  "boot",
  "ggrepel",
  "xtable",
  "scales"
)

dependencies.missing <- dependencies.list[!(dependencies.list %in% installed.packages()[,"Package"])]
if (length(dependencies.missing) > 0) {
  
  # Notify for missing libraries
  print("The following packages are required but are not installed:")
  print(dependencies.missing)
  dependencies.install <- readline(prompt = "Do you want them to be installed (Y/n)? ")
  if (any(tolower(dependencies.install) == c("y", "yes"))) {
    install.packages(dependencies.missing)
  }
}


# ==============================================================================
# Load libraries

suppressMessages(library(dplyr))
suppressMessages(library(ggplot2))
suppressMessages(library(boot))
suppressMessages(library(ggrepel))
suppressMessages(library(xtable))
suppressMessages(library(scales))


# ==============================================================================
# Read data from files

data.results <- read.csv("../data/results.csv", header = TRUE)
data.instances <- read.csv("../data/instances.csv", header = TRUE)

# Instances
instances <- as.character(unique(data.instances$NAME))


# ==============================================================================
# Mean Ranks (all algorithms)

# Algorithms compared
algorithms.labels <- c("cplex-default", "cplex-polishing", "rothberg", "maravilha")
algorithms.names  <- c("Default CPLEX", "CPLEX Pol.", "Sol.Pol.", "RbM")
names(algorithms.names) <- algorithms.labels

# Precondition data
aggdata1 <- data.results %>%
  dplyr::filter(ALGORITHM %in% algorithms.labels) %>%
  dplyr::arrange(INSTANCE, SEED, ALGORITHM) %>%
  dplyr::select(INSTANCE, ALGORITHM, SEED, OBJECTIVE) %>%
  dplyr::mutate(OBJECTIVE = round(OBJECTIVE, 4)) %>%
  dplyr::group_by(INSTANCE, SEED) %>%
  dplyr::mutate(RANK = rank(OBJECTIVE)) %>%
  dplyr::group_by(INSTANCE, ALGORITHM) %>%
  dplyr::summarise(MEAN.RANK = mean(RANK))

# Performance in terms of mean rank (box-plot)
pdf(file = "../figures/boxplot.pdf", width = 10, height = 7)
#postscript(file = "../figures/boxplot.eps", width = 10, height = 7, paper = "special", horizontal = FALSE)
fig1 <- ggplot2::ggplot(aggdata1, ggplot2::aes(x = ALGORITHM, y = MEAN.RANK, fill = ALGORITHM))
fig1 + ggplot2::geom_boxplot() + 
  ggplot2::scale_x_discrete(name = "Algorithm", limits = algorithms.labels, labels = algorithms.names) +
  ggplot2::scale_y_continuous(name = "Mean Rank") + 
  ggplot2::theme(axis.text.x = ggplot2::element_text(size = 18, angle = 0, margin = ggplot2::margin(t = 25)), 
                 axis.text.y = ggplot2::element_text(size = 18), 
                 axis.title = ggplot2::element_text(size = 20),
                 axis.title.x = ggplot2::element_text(margin = ggplot2::margin(t = 30)),
                 axis.title.y = ggplot2::element_text(margin = ggplot2::margin(r = 30)),
                 legend.position = "none", 
                 panel.background = ggplot2::element_blank(),
                 panel.border = ggplot2::element_rect(colour = "black", fill = NA, size = 1))
dev.off()


# ==============================================================================
# Wins and loses of the proposed heuristic (#variables vs. #constraints)

# Algorithms compared
algorithms.labels <- c("cplex-default", "cplex-polishing", "rothberg", "maravilha")
algorithms.names  <- c("Default CPLEX", "CPLEX Pol.", "Sol.Pol.", "RbM")
names(algorithms.names) <- algorithms.labels

# Precondition data
aggdata2 <- data.results %>%
  dplyr::arrange(INSTANCE, SEED, ALGORITHM) %>%
  dplyr::select(INSTANCE, ALGORITHM, SEED, OBJECTIVE) %>%
  dplyr::mutate(OBJECTIVE = round(OBJECTIVE, 4)) %>%
  dplyr::group_by(INSTANCE) %>%
  dplyr::slice(which(OBJECTIVE == min(OBJECTIVE))) %>%
  dplyr::group_by(INSTANCE, ALGORITHM) %>%
  dplyr::slice(which.min(OBJECTIVE))

best <- vector(mode = "list", length = length(instances))
names(best) <- instances

for (instance in instances) {
  best[[instance]] <- as.character(aggdata2$ALGORITHM[aggdata2$INSTANCE == instance])
}

# Find instances in which the proposed heuristic (Maravilha) was the winner
which.maravilha.winner <- unlist(lapply(best, function(x){"maravilha" %in% x}))
which.maravilha.solewinner <- unlist(lapply(best, function(x){"maravilha" %in% x & length(x) == 1}))

winloses <- data.instances %>%
  dplyr::arrange(NAME) %>%
  dplyr::mutate(VARS  = BINARY + INTEGER + CONTINUOUS,
                MARAVILHA.WINNER = which.maravilha.winner,
                MARAVILHA.SOLEWINNER = which.maravilha.solewinner) %>% 
  dplyr::mutate(MARAVILHA.STATUS = case_when(
    MARAVILHA.SOLEWINNER == TRUE                             ~ "RbM is the best  ",
    MARAVILHA.WINNER == TRUE & MARAVILHA.SOLEWINNER == FALSE ~ "RbM is one of the best  ",
    TRUE                                                     ~ "RbM is not the best  "
  )) %>%
  dplyr::mutate(MARAVILHA.STATUS = ordered(MARAVILHA.STATUS,
                                           levels = c("RbM is the best  ",
                                                      "RbM is one of the best  ",
                                                      "RbM is not the best  ")))

# Plot
pdf(file = "../figures/instances.pdf", width = 15, height = 10)
#postscript(file = "../figures/instances.eps", width = 15, height = 10, paper = "special", horizontal = FALSE)
fig2 <- ggplot2::ggplot(data = winloses,  mapping = aes(x = log10(VARS), 
                                                        y = log10(CONSTRAINTS), 
                                                        label = NAME,
                                                        shape = MARAVILHA.STATUS,
                                                        color = MARAVILHA.STATUS))
fig2 + ggplot2::geom_point(size = 5) +
  ggplot2::scale_shape_manual(name = "Legend:", values = c(17, 24, 21) ) + 
  ggplot2::scale_color_manual(name = "Legend:", values = c(hue_pal()(3)[3], hue_pal()(3)[3], hue_pal()(3)[1])) + 
  ggplot2::labs(x = expression(log[10](variables)), y = expression(log[10](constraints))) + 
  ggrepel::geom_text_repel(point.padding = unit(0.5, "char"), colour = "#3a3a3a", size = 5) + 
  ggplot2::theme(axis.text = ggplot2::element_text(size = 16), 
                 axis.title = ggplot2::element_text(size = 18), 
                 legend.text = ggplot2::element_text(size = 18),
                 legend.title = ggplot2::element_text(size = 18),
                 panel.background = ggplot2::element_blank(),
                 panel.border = ggplot2::element_rect(colour = "black", fill = NA, size = 1)) +
  ggplot2::theme(legend.position = "bottom", 
                 legend.direction = "horizontal", 
                 legend.key = ggplot2::element_blank())
dev.off()


# ==============================================================================
# Inference using bootstrap CIs (Bonferroni-corrected)

# Algorithms compared
algorithms.labels <- c("cplex-default", "cplex-polishing", "rothberg", "maravilha")
algorithms.names  <- c("Default CPLEX", "CPLEX Pol.", "Sol.Pol.", "RbM")
names(algorithms.names) <- algorithms.labels

# Precondition data
aggdata3 <- data.results %>%
  dplyr::filter(ALGORITHM %in% algorithms.labels) %>%
  dplyr::arrange(INSTANCE, SEED, ALGORITHM) %>%
  dplyr::select(INSTANCE, ALGORITHM, SEED, OBJECTIVE) %>%
  dplyr::mutate(OBJECTIVE = round(OBJECTIVE, 4)) %>%
  dplyr::group_by(INSTANCE, SEED) %>%
  dplyr::mutate(RANK = rank(OBJECTIVE)) %>%
  dplyr::group_by(INSTANCE, ALGORITHM) %>%
  dplyr::summarise(MEAN.RANK = mean(RANK))

# Get paired differences
PW.diff <- with(aggdata3,
                data.frame(MvC = MEAN.RANK[ALGORITHM == "maravilha"] - MEAN.RANK[ALGORITHM == "cplex-default"],
                           MvR = MEAN.RANK[ALGORITHM == "maravilha"] - MEAN.RANK[ALGORITHM == "rothberg"],
                           MvP = MEAN.RANK[ALGORITHM == "maravilha"] - MEAN.RANK[ALGORITHM == "cplex-polishing"],
                           RvC = MEAN.RANK[ALGORITHM == "rothberg"] - MEAN.RANK[ALGORITHM == "cplex-default"],
                           RvP = MEAN.RANK[ALGORITHM == "rothberg"] - MEAN.RANK[ALGORITHM == "cplex-polishing"],
                           PvC = MEAN.RANK[ALGORITHM == "cplex-polishing"] - MEAN.RANK[ALGORITHM == "cplex-default"]))

# Bootstrap paired differences
bootfun <- function(x, i) { mean(x[i]) }
my.boot <- list(MvC = boot::boot(data      = PW.diff$MvC, 
                                 statistic = bootfun, 
                                 R         = 999),
                MvR = boot::boot(data      = PW.diff$MvR, 
                                 statistic = bootfun, 
                                 R         = 999),
                MvP = boot::boot(data      = PW.diff$MvP, 
                                 statistic = bootfun, 
                                 R         = 999),
                RvC = boot::boot(data      = PW.diff$RvC, 
                                 statistic = bootfun, 
                                 R         = 999),
                RvP = boot::boot(data      = PW.diff$RvP, 
                                 statistic = bootfun, 
                                 R         = 999),
                PvC = boot::boot(data      = PW.diff$PvC, 
                                 statistic = bootfun, 
                                 R         = 999))

# Get bootstrap CIs and empirical sample distributions of the means
my.ci.df <- data.frame(Comparison = c("RbM vs. Default CPLEX",
                                      "RbM vs. Sol.Pol.",
                                      "RbM vs. CPLEX Pol.",
                                      "Sol.Pol. vs. Default CPLEX",
                                      "Sol.Pol. vs. CPLEX Pol.",
                                      "CPLEX Pol. vs Default CPLEX"),
                       Est = numeric(3),
                       CIL = numeric(3),
                       CIU = numeric(3))

conf.bon <- 1 - (0.05 / length(my.boot))
for (i in 1:length(my.boot)) {
  my.bootci <- boot::boot.ci(my.boot[[i]], conf = conf.bon, type = "perc")
  my.ci.df[i, 2]   <- my.bootci$t0
  my.ci.df[i, 3:4] <- my.bootci[[4]][4:5]
}

# Plot
pdf(file = "../figures/comparisons.pdf", width = 15, height = 5)
#postscript(file = "../figures/comparisons.eps", width = 15, height = 5, paper = "special", horizontal = FALSE)
fig3 <- ggplot(my.ci.df, aes(x = Comparison, y = Est, ymax = CIU, ymin = CIL))
fig3 + geom_hline(yintercept = 0, size = 1.3, col = 2, linetype = 2) + 
  geom_pointrange(fatten = 5, size = 1.3) + coord_flip() + 
  xlab("Comparison") +
  ylab("Mean difference in ranks") +
  ggplot2::theme(axis.text = ggplot2::element_text(size = 16), 
                 axis.title = ggplot2::element_text(size = 16), 
                 axis.title.x = ggplot2::element_text(margin = ggplot2::margin(t = 30)),
                 axis.title.y = ggplot2::element_text(margin = ggplot2::margin(r = 20)),
                 legend.position = "none",
                 panel.background = ggplot2::element_blank(),
                 panel.border = ggplot2::element_rect(colour = "black", fill = NA, size = 1))
dev.off()


# ==============================================================================
# Generate LaTeX tables

baseline <- data.results %>%
  dplyr::filter(ALGORITHM == "cplex-default") %>%
  dplyr::group_by(INSTANCE) %>%
  dplyr::mutate(OBJECTIVE = min(round(OBJECTIVE, 4))) %>%
  dplyr::filter(!duplicated(INSTANCE)) %>%
  dplyr::select(INSTANCE, ALGORITHM, OBJECTIVE) %>%
  dplyr::arrange(INSTANCE)

# LaTeX table with best results
data.table.best <- data.results %>%
  dplyr::filter(ALGORITHM %in% c("cplex-default", "maravilha", "rothberg", "cplex-polishing")) %>%
  dplyr::select(INSTANCE, ALGORITHM, OBJECTIVE) %>%
  dplyr::group_by(INSTANCE, ALGORITHM) %>%
  dplyr::mutate(BEST.OBJECTIVE = min(round(OBJECTIVE, 4))) %>%
  dplyr::group_by(INSTANCE, ALGORITHM) %>%
  dplyr::mutate(WORST.OBJECTIVE = max(round(OBJECTIVE, 4))) %>%
  dplyr::select(INSTANCE, ALGORITHM, BEST.OBJECTIVE, WORST.OBJECTIVE) %>%
  dplyr::group_by(INSTANCE) %>%
  dplyr::filter(!duplicated(ALGORITHM)) %>%
  dplyr::arrange(INSTANCE, ALGORITHM) %>%
  dplyr::mutate(BASELINE = rep(BEST.OBJECTIVE[ALGORITHM == "cplex-default"], each = length(unique(ALGORITHM))),
                IMPROV = ((BASELINE - BEST.OBJECTIVE) / abs(BASELINE)) * 100)

my.table.best <- with(data.table.best,
                      cbind(WORST.CPLEX = WORST.OBJECTIVE[ALGORITHM == "cplex-default"],
                            BEST.CPLEX = BEST.OBJECTIVE[ALGORITHM == "cplex-default"],
                            WORST.MARAVILHA = WORST.OBJECTIVE[ALGORITHM == "maravilha"],
                            BEST.MARAVILHA = BEST.OBJECTIVE[ALGORITHM == "maravilha"],
                            IMP.MARAVILHA = IMPROV[ALGORITHM == "maravilha"],
                            WORST.ROTHBERG = WORST.OBJECTIVE[ALGORITHM == "rothberg"],
                            BEST.ROTHBERG = BEST.OBJECTIVE[ALGORITHM == "rothberg"],
                            IMP.ROTHBERG = IMPROV[ALGORITHM == "rothberg"],
                            WORST.CPLEXPOL = WORST.OBJECTIVE[ALGORITHM == "cplex-polishing"],
                            BEST.CPLEXPOL = BEST.OBJECTIVE[ALGORITHM == "cplex-polishing"],
                            IMP.CPLEXPOL = IMPROV[ALGORITHM == "cplex-polishing"]))
rownames(my.table.best) <- sort(data.instances$NAME)
xtable(my.table.best, digits = c(6,2,2,2,2,6,2,2,6,2,2,6))


# LaTeX table with a summary of results
data.table.summary <- data.results %>%
  dplyr::filter(ALGORITHM %in% c("cplex-default", "maravilha", "rothberg", "cplex-polishing")) %>%
  dplyr::select(INSTANCE, ALGORITHM, OBJECTIVE, SEED) %>%
  dplyr::mutate(OBJECTIVE = round(OBJECTIVE, 4)) %>%
  dplyr::arrange(ALGORITHM, SEED, INSTANCE) %>%
  dplyr::mutate(BASELINE = rep(OBJECTIVE[ALGORITHM == "cplex-default"], times = length(unique(ALGORITHM))),
                IMPROV = ((BASELINE - OBJECTIVE) / abs(BASELINE)) * 100) %>%
  dplyr::group_by(ALGORITHM) %>%
  dplyr::mutate(IMPROV.MIN = min(IMPROV),
                IMPROV.MAX = max(IMPROV),
                IMPROV.AVG = mean(IMPROV)) %>%
  dplyr::filter(!duplicated(ALGORITHM)) %>%
  dplyr::filter(ALGORITHM %in% c("maravilha", "rothberg", "cplex-polishing")) %>%
  dplyr::select(ALGORITHM, IMPROV.MIN, IMPROV.MAX, IMPROV.AVG) %>%
  dplyr::arrange(ALGORITHM)

my.table.summary <- with(data.table.summary,
                         cbind(IMPROV.MIN = IMPROV.MIN,
                               IMPROV.MAX = IMPROV.MAX,
                               IMPROV.AVG = IMPROV.AVG))
rownames(my.table.summary) <- data.table.summary$ALGORITHM
xtable(my.table.summary, digits = c(6,6,6,6))
